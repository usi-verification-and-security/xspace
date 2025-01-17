#include "Framework.h"

#include "Config.h"
#include "Parse.h"
#include "Preprocess.h"
#include "Print.h"
#include "expand/Expand.h"

#include <xspace/common/Macro.h>

// for the destructor
#include "expand/strategy/Strategy.h"

#include <verifiers/Verifier.h>

namespace xspace {
Framework::Framework() : Framework(Config{}) {}

Framework::Framework(Config const & config) : configPtr{MAKE_UNIQUE(config)} {
    expandPtr = std::make_unique<Expand>(*this);
    printPtr = std::make_unique<Print>(*this);
}

Framework::Framework(Config const & config, std::unique_ptr<xai::nn::NNet> network) : Framework(config) {
    setNetwork(std::move(network));
}

Framework::Framework(Config const & config, std::unique_ptr<xai::nn::NNet> network, std::istream & expandStrategiesSpec)
    : Framework(config, std::move(network)) {
    setExpand(expandStrategiesSpec);
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

void Framework::setExpand(std::istream & strategiesSpec) {
    assert(expandPtr);
    expandPtr->setStrategies(strategiesSpec);
    expandPtr->setVerifier();
}

void Framework::setExpand(std::string_view verifierName, std::istream & strategiesSpec) {
    assert(expandPtr);
    expandPtr->setStrategies(strategiesSpec);
    expandPtr->setVerifier(verifierName);
}

Explanations Framework::explain(Dataset & data) {
    Preprocess preprocess{*this, data};
    auto explanations = preprocess.makeExplanationsFromSamples();

    expand(explanations, data);

    return explanations;
}

Explanations Framework::expand(std::string_view fileName, Dataset & data) {
    Preprocess preprocess{*this, data};
    Parse parse{*this};
    //+ only interval explanations supported
    auto explanations = parse.parseIntervalExplanations(fileName, data);

    expand(explanations, data);

    return explanations;
}

void Framework::expand(Explanations & explanations, Dataset const & data) {
    (*expandPtr)(explanations, data);
}
} // namespace xspace
