#include "Expand.h"

#include "../Config.h"
#include "../Preprocess.h"
#include "../Print.h"
#include "../explanation/Explanation.h"
#include "strategy/Strategies.h"

#include <xspace/common/Core.h>
#include <xspace/common/String.h>

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
Framework::Expand::Expand(Framework & fw) : framework{fw} {}

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

void Framework::Expand::setVerifier(std::string_view name) {
    setVerifier(makeVerifier(name));
}

void Framework::Expand::setVerifier(std::unique_ptr<xai::verifiers::Verifier> vf) {
    assert(vf);
    verifierPtr = std::move(vf);
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

    //+ move this parsing to particular strategies
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

        if (nameLower == "trial") {
            TrialAndErrorStrategy::Config conf;
            bool maxAttemptsFollows = false;
            while (not params.empty()) {
                auto param = std::move(params.front());
                params.pop();
                if (maxAttemptsFollows) {
                    conf.maxAttempts = std::stoi(param);
                    break;
                }
                auto const paramLower = toLower(param);
                if (paramLower == "n") {
                    maxAttemptsFollows = true;
                    continue;
                }

                throwInvalidParameter(name, param);
            }
            checkAdditionalParameters(line, params);
            addStrategy(std::make_unique<TrialAndErrorStrategy>(*this, conf, defaultVarOrder));
            continue;
        }

        if (nameLower == "itp") {
            OpenSMTInterpolationStrategy::Config conf;
            while (not params.empty()) {
                auto param = std::move(params.front());
                params.pop();
                auto const paramLower = toLower(param);
                if (paramLower == "weak") {
                    conf.boolInterpolationAlg = OpenSMTInterpolationStrategy::BoolInterpolationAlg::weak;
                    conf.arithInterpolationAlg = OpenSMTInterpolationStrategy::ArithInterpolationAlg::weak;
                    break;
                }

                if (paramLower == "strong") {
                    conf.boolInterpolationAlg = OpenSMTInterpolationStrategy::BoolInterpolationAlg::strong;
                    conf.arithInterpolationAlg = OpenSMTInterpolationStrategy::ArithInterpolationAlg::strong;
                    break;
                }

                if (paramLower == "weaker") {
                    conf.boolInterpolationAlg = OpenSMTInterpolationStrategy::BoolInterpolationAlg::weak;
                    conf.arithInterpolationAlg = OpenSMTInterpolationStrategy::ArithInterpolationAlg::weaker;
                    break;
                }

                if (paramLower == "stronger") {
                    conf.boolInterpolationAlg = OpenSMTInterpolationStrategy::BoolInterpolationAlg::strong;
                    conf.arithInterpolationAlg = OpenSMTInterpolationStrategy::ArithInterpolationAlg::stronger;
                    break;
                }

                if (paramLower == "bweak") {
                    conf.boolInterpolationAlg = OpenSMTInterpolationStrategy::BoolInterpolationAlg::weak;
                    continue;
                }

                if (paramLower == "bstrong") {
                    conf.boolInterpolationAlg = OpenSMTInterpolationStrategy::BoolInterpolationAlg::strong;
                    continue;
                }

                if (paramLower == "aweak") {
                    conf.arithInterpolationAlg = OpenSMTInterpolationStrategy::ArithInterpolationAlg::weak;
                    continue;
                }

                if (paramLower == "astrong") {
                    conf.arithInterpolationAlg = OpenSMTInterpolationStrategy::ArithInterpolationAlg::strong;
                    continue;
                }

                if (paramLower == "aweaker") {
                    conf.arithInterpolationAlg = OpenSMTInterpolationStrategy::ArithInterpolationAlg::weaker;
                    continue;
                }

                if (paramLower == "astronger") {
                    conf.arithInterpolationAlg = OpenSMTInterpolationStrategy::ArithInterpolationAlg::stronger;
                    continue;
                }

                throwInvalidParameter(name, param);
            }
            checkAdditionalParameters(line, params);
            addStrategy(std::make_unique<OpenSMTInterpolationStrategy>(*this, conf, defaultVarOrder));
            continue;
        }

        throw std::invalid_argument{"Unrecognized strategy name: "s + name};
    }
}

void Framework::Expand::addStrategy(std::unique_ptr<Strategy> strategy) {
    isAbductiveOnly &= strategy->isAbductiveOnly();

    strategies.push_back(std::move(strategy));
}

void Framework::Expand::operator()(Explanations & explanations, Dataset & data) {
    assert(not strategies.empty());

    auto const & config = framework.getConfig();

    Print & print = *framework.printPtr;
    bool const printingStats = not print.ignoringStats();
    bool const printingExplanations = not print.ignoringExplanations();
    auto & cexp = print.explanations();

    if (printingStats) { printStatsHead(data); }

    initVerifier();

    //? This incrementality does not seem to work
    // assertModel();

    //++ allow also other views based on config
    auto indices = data.getSampleIndices();
    assert(indices.size() <= data.size());
    assert(explanations.size() == data.size());
    if (config.limitingMaxSamples()) {
        auto const maxSamples = config.getMaxSamples();
        assert(maxSamples > 0);
        if (maxSamples < indices.size()) { indices.resize(maxSamples); }
    }

    for (auto idx : indices) {
        //++ This should ideally be outside of the loop
        assertModel();

        auto const & output = data.getComputedOutput(idx);
        assertClassification(output);

        auto & explanationPtr = explanations[idx];
        for (auto & strategy : strategies) {
            verifierPtr->push();
            strategy->execute(explanationPtr);
            verifierPtr->pop();
        }

        //+ get rid of the conditionals
        auto & explanation = *explanationPtr;
        if (printingStats) { printStats(explanation, data, idx); }
        if (printingExplanations) {
            explanation.print(cexp);
            cexp << std::endl;
        }

        resetClassification();

        //++ This should not ideally be necessary
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

void Framework::Expand::assertClassification(Dataset::Output const & output) {
    verifierPtr->push();

    auto & network = framework.getNetwork();
    auto const outputLayerIndex = network.getNumLayers() - 1;

    auto const label = output.classificationLabel;
    auto const & outputValues = output.values;

    if (not Preprocess::isBinaryClassification(outputValues)) {
        assert(outputValues.size() == network.getLayerSize(outputLayerIndex));
        verifierPtr->addClassificationConstraint(label, 0);
        return;
    }

    // With single output value, the flip in classification means flipping the value across 0
    constexpr Float threshold = 0.015625f;
    [[maybe_unused]] Float const val = outputValues.front();
    assert(Preprocess::computeBinaryClassificationLabel(outputValues) == label);
    assert(label == 0 || label == 1);
    if (label == 1) {
        assert(val >= 0);
        // <= -threshold
        verifierPtr->addUpperBound(outputLayerIndex, 0, -threshold);
    } else {
        assert(val < 0);
        // >= threshold
        verifierPtr->addLowerBound(outputLayerIndex, 0, threshold);
    }
}

void Framework::Expand::resetClassification() {
    verifierPtr->pop();
    verifierPtr->resetSample();
}

void Framework::Expand::printStatsHead(Dataset const & data) const {
    Print const & print = *framework.printPtr;
    assert(not print.ignoringStats());
    auto & cstats = print.stats();

    std::size_t const size = data.size();
    cstats << "Dataset size: " << size << '\n';
    cstats << "Number of variables: " << framework.varSize() << '\n';
    cstats << std::string(60, '-') << '\n';
}

void Framework::Expand::printStats(Explanation const & explanation, Dataset const & data,
                                   Dataset::Sample::Idx idx) const {
    Print const & print = *framework.printPtr;
    assert(not print.ignoringStats());
    auto & cstats = print.stats();
    auto const defaultPrecision = cstats.precision();

    std::size_t const varSize = framework.varSize();
    std::size_t const expVarSize = explanation.varSize();
    assert(expVarSize <= varSize);

    std::size_t const dataSize = data.size();
    auto const & sample = data.getSample(idx);
    auto const & expClass = data.getExpectedClassification(idx).label;
    auto const & compClass = data.getComputedOutput(idx).classificationLabel;

    std::size_t const fixedCount = explanation.getFixedCount();
    assert(fixedCount <= expVarSize);

    cstats << '\n';
    cstats << "sample [" << idx + 1 << '/' << dataSize << "]: " << sample << '\n';
    cstats << "expected output: " << expClass << '\n';
    cstats << "computed output: " << compClass << '\n';
    cstats << "#checks: " << verifierPtr->getChecksCount() << '\n';
    cstats << "explanation size: " << expVarSize << '/' << varSize << std::endl;

    assert(explanation.getRelativeVolumeSkipFixed() > 0);
    assert(explanation.getRelativeVolumeSkipFixed() <= 1);
    assert((explanation.getRelativeVolumeSkipFixed() < 1) == (fixedCount < expVarSize));
    if (isAbductiveOnly) { return; }

    Float const relVolume = explanation.getRelativeVolumeSkipFixed();

    cstats << "fixed features: " << fixedCount << '/' << varSize << '\n';
    cstats << "relVolume*: " << std::setprecision(1) << (relVolume * 100) << "%" << std::setprecision(defaultPrecision)
           << std::endl;
}
} // namespace xspace
