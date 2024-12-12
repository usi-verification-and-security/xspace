#include "Expand.h"

#include "../Config.h"
#include "../Print.h"
#include "../explanation/Explanation.h"
#include "strategy/Strategy.h"

#include <xspace/common/String.h>
#include <xspace/nn/Dataset.h>

#include <verifiers/Verifier.h>
#include <verifiers/opensmt/OpenSMTVerifier.h>
#ifdef MARABOU
#include <verifiers/marabou/MarabouVerifier.h>
#endif

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <numeric>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>

namespace xspace {
std::unique_ptr<xai::verifiers::Verifier> Framework::Expand::makeVerifier(std::string_view name) {
    if (toLower(name) == "opensmt") {
        return std::make_unique<xai::verifiers::OpenSMTVerifier>();
#ifdef MARABOU
    } else if (toLower(name) == "marabou") {
        return std::make_unique<xai::verifiers::MarabouVerifier>();
#endif
    }

    throw std::invalid_argument{"Unrecognized verifier name: "s + std::string{name}};
}

void Framework::Expand::setStrategies(std::istream & is) {
    static auto const checkAdditionalParameters = [](std::string const & line, auto const & params) {
        if (params.empty()) return;
        throw std::invalid_argument{"Additional parameters of strategy: "s + line};
    };
    static auto const throwInvalidParameter = [](std::string const & name, auto const & param) {
        throw std::invalid_argument{"Invalid parameter of strategy "s + name + ": " + param};
    };

    auto const & config = framework.getConfig();

    VarOrdering defaultVarOrder{};
    if (config.isReverseVarOrdering()) { defaultVarOrder.type = VarOrdering::Type::reverse; }

    std::string line;
    while (std::getline(is, line, ',')) {
        std::istringstream issLine{line};
        std::string name;
        issLine >> name;
        std::queue<std::string> params;
        for (std::string param; issLine >> param;) {
            params.push(std::move(param));
        }

        auto const nameLower = toLower(name);
        if (nameLower == "abductive") {
            checkAdditionalParameters(line, params);
            addStrategy(std::make_unique<AbductiveStrategy>(*this, defaultVarOrder));
            continue;
        }

        if (nameLower == "ucore") {
            UnsatCoreStrategy::Config conf;
            while (not params.empty()) {
                auto param = std::move(params.front());
                params.pop();
                auto const paramLower = toLower(param);
                if (paramLower == "sample") {
                    conf.splitEq = false;
                    break;
                }

                if (paramLower == "interval") {
                    conf.splitEq = true;
                    break;
                }

                throwInvalidParameter(name, param);
            }
            checkAdditionalParameters(line, params);
            addStrategy(std::make_unique<UnsatCoreStrategy>(*this, conf, defaultVarOrder));
            continue;
        }

        throw std::invalid_argument{"Unrecognized strategy name: "s + name};
    }
}

void Framework::Expand::addStrategy(std::unique_ptr<Strategy> strategy) {
    isAbductiveOnly &= strategy->isAbductiveOnly();

    strategies.push_back(std::move(strategy));
}

void Framework::Expand::operator()(std::vector<IntervalExplanation> & explanations, Dataset const & data) {
    assert(not strategies.empty());

    Print & print = *framework.printPtr;
    bool const printingStats = not print.ignoringStats();
    bool const printingExplanations = not print.ignoringExplanations();
    auto & cexp = print.explanations();

    auto const & config = framework.getConfig();
    auto const & expPrintFormat = config.getPrintingExplanationsFormat();

    if (printingStats) { printStatsHead(data); }

    initVerifier();

    //? This incrementality does not seem to work
    // assertModel();

    std::size_t const size = explanations.size();
    assert(data.getSamples().size() == size);
    assert(data.getOutputs().size() == size);
    assert(outputs.size() == size);
    for (std::size_t i = 0; i < size; ++i) {
        //+ This should ideally be outside of the loop
        assertModel();

        auto const & output = outputs[i];
        assertClassification(output);

        auto & explanation = explanations[i];
        for (auto & strategy : strategies) {
            strategy->execute(explanation);
        }

        if (printingStats) { printStats(explanation, data, i); }
        if (printingExplanations) {
            explanation.print(cexp, expPrintFormat);
            cexp << std::endl;
        }

        resetClassification();

        //+ This should not ideally be necessary
        resetModel();
    }
}

void Framework::Expand::initVerifier() {
    assert(verifierPtr);
    verifierPtr->init();
}

void Framework::Expand::assertModel() {
    auto & nn = framework.getNetwork();
    verifierPtr->loadModel(nn);
}

void Framework::Expand::resetModel() {
    verifierPtr->reset();
}

void Framework::Expand::assertClassification(Output const & output) {
    verifierPtr->push();

    auto & network = framework.getNetwork();
    auto const outputLayerIndex = network.getNumLayers() - 1;
    auto const & outputValues = output.values;
    if (outputValues.size() == 1) {
        // With single output value, the flip in classification means flipping the value across certain threshold
        constexpr Float threshold = 0;
        constexpr Float precision = 0.015625f;
        Float const outputValue = outputValues.front();
        if (outputValue >= threshold) {
            verifierPtr->addUpperBound(outputLayerIndex, 0, threshold - precision);
        } else {
            verifierPtr->addLowerBound(outputLayerIndex, 0, threshold + precision);
        }
        return;
    }

    [[maybe_unused]] auto const outputLayerSize = network.getLayerSize(outputLayerIndex);
    assert(outputLayerSize == outputValues.size());
    verifierPtr->addClassificationConstraint(output.classifiedIdx, 0);
}

void Framework::Expand::resetClassification() {
    verifierPtr->pop();
    verifierPtr->resetSample();
}

void Framework::Expand::printStatsHead(Dataset const & data) const {
    Print const & print = *framework.printPtr;
    assert(not print.ignoringStats());
    auto & cstats = print.stats();

    std::size_t const size = data.getSamples().size();
    assert(size == data.getOutputs().size());
    assert(size == outputs.size());
    cstats << "Dataset size: " << size << '\n';
    cstats << "Number of variables: " << framework.varSize() << '\n';
    cstats << std::string(60, '-') << '\n';
}

void Framework::Expand::printStats(IntervalExplanation const & explanation, Dataset const & data, std::size_t i) const {
    Print const & print = *framework.printPtr;
    assert(not print.ignoringStats());
    auto & cstats = print.stats();
    auto const defaultPrecision = cstats.precision();

    std::size_t const varSize = framework.varSize();
    std::size_t const expSize = explanation.size();
    assert(expSize <= varSize);

    auto const & samples = data.getSamples();
    auto const & expOutputs = data.getOutputs();
    std::size_t const size = samples.size();
    assert(size == expOutputs.size());
    assert(size == outputs.size());
    auto const & sample = samples[i];
    auto const & expOutput = expOutputs[i];
    auto const & output = outputs[i];

    std::size_t const fixedCount = explanation.getFixedCount();
    assert(fixedCount <= expSize);

    cstats << '\n';
    cstats << "sample [" << i + 1 << '/' << size << "]: " << sample << '\n';
    cstats << "expected output: " << expOutput << '\n';
    cstats << "computed output: " << output.classifiedIdx << '\n';
    cstats << "#checks: " << verifierPtr->getChecksCount() << '\n';
    cstats << "explanation size: " << expSize << '/' << varSize << std::endl;

    assert(explanation.getRelativeVolumeSkipFixed() > 0);
    assert(explanation.getRelativeVolumeSkipFixed() <= 1);
    assert((explanation.getRelativeVolumeSkipFixed() < 1) == (fixedCount < expSize));
    if (isAbductiveOnly) { return; }

    Float const relVolume = explanation.getRelativeVolumeSkipFixed();

    cstats << "fixed features: " << fixedCount << '/' << varSize << '\n';
    cstats << "relVolume*: " << std::setprecision(1) << (relVolume * 100) << "%" << std::setprecision(defaultPrecision)
           << std::endl;
}
} // namespace xspace
