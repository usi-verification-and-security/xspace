#include "Framework.h"

#include "Config.h"
#include "Print.h"
#include "expand/Expand.h"
#include "expand/strategy/Strategy.h"

#include <xspace/explanation/Explanation.h>
#include <xspace/nn/Dataset.h>

#include <verifiers/Verifier.h>

#include <algorithm>
#include <type_traits>

namespace xspace {
Framework::Framework() : Framework(Config{}) {}

Framework::Framework(Config const & config) : configPtr{std::make_unique<Config>(config)} {
    expandPtr = std::make_unique<Expand>(*this);
    printPtr = std::make_unique<Print>(*this);
}

Framework::~Framework() = default;

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

void Framework::setVerifier(std::string_view name) {
    expandPtr->setVerifier(name);
}

void Framework::setExpandStrategies(std::istream & is) {
    expandPtr->setStrategies(is);
}

std::vector<IntervalExplanation> Framework::explain(Dataset const & data) {
    auto & expand = *expandPtr;

    std::vector<IntervalExplanation> explanations = encodeSamples(data);
    expand(explanations, data);
    return explanations;
}

std::vector<IntervalExplanation> Framework::encodeSamples(Dataset const & data) {
    auto & samples = data.getSamples();
    assert(not samples.empty());
    assert(not varNames.empty());

    auto const & nn = getNetwork();

    std::size_t const size = samples.size();
    assert(size == data.size());
    std::vector<IntervalExplanation> explanations;
    Expand::Outputs outputs;
    explanations.reserve(size);
    outputs.reserve(size);
    std::size_t const vSize = varSize();
    for (auto const & sample : samples) {
        assert(sample.size() == vSize);

        IntervalExplanation explanation{*this};
        for (VarIdx idx = 0; idx < vSize; ++idx) {
            Float const val = sample[idx];
            EqBound bnd{val};
            explanation.insertBound(idx, std::move(bnd));
        }
        explanations.push_back(std::move(explanation));

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
