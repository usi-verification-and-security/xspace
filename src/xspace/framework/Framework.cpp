#include "Framework.h"

#include "Config.h"
#include "Print.h"
#include "expand/Expand.h"
#include "expand/strategy/Strategy.h"
#include "explanation/Explanation.h"
#include "explanation/IntervalExplanation.h"

#include <xspace/common/Macro.h>
#include <xspace/nn/Dataset.h>

#include <verifiers/Verifier.h>

#include <algorithm>
#include <type_traits>

namespace xspace {
Framework::Framework() : Framework(Config{}) {}

Framework::Framework(Config const & config) : configPtr{MAKE_UNIQUE(config)} {
    expandPtr = std::make_unique<Expand>(*this);
    printPtr = std::make_unique<Print>(*this);
}

Framework::Framework(Config const & config, std::unique_ptr<xai::nn::NNet> network) : Framework(config) {
    setNetwork(std::move(network));
}

Framework::Framework(Config const & config, std::unique_ptr<xai::nn::NNet> network, std::string_view verifierName,
                     std::istream & expandStrategiesSpec)
    : Framework(config, std::move(network)) {
    setExpand(verifierName, expandStrategiesSpec);
}

Framework::~Framework() = default;

void Framework::setConfig(Config const & config) {
    configPtr = MAKE_UNIQUE(config);
}

void Framework::setNetwork(std::unique_ptr<xai::nn::NNet> nn) {
    assert(nn);
    networkPtr = std::move(nn);

    auto & network = *networkPtr;
    std::size_t const size = network.getInputSize();
    varNames.reserve(size);
    domainIntervals.reserve(size);
    for (VarIdx idx = 0; idx < size; ++idx) {
        varNames.emplace_back(makeVarName(idx));
        domainIntervals.emplace_back(network.getInputLowerBound(idx), network.getInputUpperBound(idx));
    }
}

void Framework::setExpand(std::string_view verifierName, std::istream & strategiesSpec) {
    assert(expandPtr);
    expandPtr->setVerifier(verifierName);
    expandPtr->setStrategies(strategiesSpec);
}

Explanations Framework::explain(Dataset const & data) {
    auto & expand = *expandPtr;

    auto explanations = encodeSamples(data);
    expand(explanations, data);
    return explanations;
}

Explanations Framework::encodeSamples(Dataset const & data) {
    auto & samples = data.getSamples();
    assert(not samples.empty());
    assert(not varNames.empty());

    auto const & nn = getNetwork();

    std::size_t const size = samples.size();
    assert(size == data.size());
    Explanations explanations;
    Expand::Outputs outputs;
    explanations.reserve(size);
    outputs.reserve(size);
    std::size_t const vSize = varSize();
    for (auto const & sample : samples) {
        assert(sample.size() == vSize);

        IntervalExplanation iexplanation{*this};
        for (VarIdx idx = 0; idx < vSize; ++idx) {
            Float const val = sample[idx];
            EqBound bnd{val};
            iexplanation.insertBound(idx, std::move(bnd));
        }
        explanations.push_back(MAKE_UNIQUE(std::move(iexplanation)));

        static_assert(std::is_base_of_v<xai::nn::NNet::input_t, Dataset::Sample>);
        static_assert(std::is_base_of_v<xai::nn::NNet::output_t, Expand::Output::Values>);
        Expand::Output::Values outputValues = xai::nn::computeOutput(sample, nn);
        assert(not outputValues.empty());
        //++ make function for this
        VarIdx label = (outputValues.size() == 1)
                         ? (outputValues.front() >= 0)
                         : std::distance(outputValues.begin(), std::ranges::max_element(outputValues));
        outputs.emplace_back(std::move(outputValues), label);
    }

    expandPtr->setOutputs(std::move(outputs));

    return explanations;
}
} // namespace xspace
