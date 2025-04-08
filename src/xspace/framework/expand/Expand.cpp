#include "Expand.h"

#include "../Config.h"
#include "../Preprocess.h"
#include "../Print.h"
#include "../explanation/Explanation.h"
#include "strategy/Factory.h"
#include "strategy/Strategy.h"

#include <xspace/common/Core.h>
#include <xspace/common/String.h>

#include <verifiers/Verifier.h>
#include <verifiers/opensmt/OpenSMTVerifier.h>
#ifdef MARABOU
#include <verifiers/marabou/MarabouVerifier.h>
#endif

#include <api/MainSolver.h>

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <random>
#include <stdexcept>
#include <string>

namespace xspace {
Framework::Expand::Expand(Framework & fw) : framework{fw} {}

void Framework::Expand::setStrategies(std::istream & is) {
    // pipe character '|' reserved for disjunctions
    static constexpr char strategyDelim = ';';

    auto const & config = framework.getConfig();

    VarOrdering defaultVarOrder{};
    if (config.isReverseVarOrdering()) { defaultVarOrder.type = VarOrdering::Type::reverse; }

    Strategy::Factory factory{*this, defaultVarOrder};

    std::string line;
    while (std::getline(is, line, strategyDelim)) {
        auto strategyPtr = factory.parse(line);
        addStrategy(std::move(strategyPtr));
    }
}

void Framework::Expand::addStrategy(std::unique_ptr<Strategy> strategy) {
    requiresSMTSolver |= strategy->requiresSMTSolver();

    strategies.push_back(std::move(strategy));
}

std::unique_ptr<xai::verifiers::Verifier> Framework::Expand::makeVerifier(std::string_view name) const {
#ifdef MARABOU
    if (name.empty() and not requiresSMTSolver) { name = "marabou"sv; }
#endif

    if (name.empty() or toLower(name) == "opensmt") {
        return std::make_unique<xai::verifiers::OpenSMTVerifier>();
#ifdef MARABOU
    } else if (toLower(name) == "marabou") {
        return std::make_unique<xai::verifiers::MarabouVerifier>();
#endif
    }

    throw std::invalid_argument{"Unrecognized verifier name: "s + std::string{name}};
}

void Framework::Expand::setVerifier() {
    setVerifier(""sv);
}

void Framework::Expand::setVerifier(std::string_view name) {
    setVerifier(makeVerifier(name));
}

void Framework::Expand::setVerifier(std::unique_ptr<xai::verifiers::Verifier> vf) {
    assert(vf);
    verifierPtr = std::move(vf);
}

Dataset::SampleIndices Framework::Expand::makeSampleIndices(Dataset const & data) const {
    auto indices = getSampleIndices(data);
    assert(indices.size() <= data.size());

    auto const & config = framework.getConfig();

    if (config.shufflingSamples()) { std::ranges::shuffle(indices, std::default_random_engine{}); }

    if (config.limitingMaxSamples()) {
        auto const maxSamples = config.getMaxSamples();
        assert(maxSamples > 0);
        if (maxSamples < indices.size()) { indices.resize(maxSamples); }
    }

    return indices;
}

Dataset::SampleIndices Framework::Expand::getSampleIndices(Dataset const & data) const {
    auto const & config = framework.getConfig();

    if (not config.filteringSamplesOfExpectedClass()) {
        if (config.filteringCorrectSamples()) { return data.getCorrectSampleIndices(); }
        if (config.filteringIncorrectSamples()) { return data.getIncorrectSampleIndices(); }
        return data.getSampleIndices();
    }

    auto const & label = config.getSamplesExpectedClassFilter().label;
    if (config.filteringCorrectSamples()) { return data.getCorrectSampleIndicesOfExpectedClass(label); }
    if (config.filteringIncorrectSamples()) { return data.getIncorrectSampleIndicesOfExpectedClass(label); }
    return data.getSampleIndicesOfExpectedClass(label);
}

void Framework::Expand::operator()(Explanations & explanations, Dataset const & data) {
    assert(not strategies.empty());

    assert(explanations.size() <= data.size());
    //+ if we start from file where filtering (and possibly also shuffling) happened,
    // we would have to sync the explanations with the sample points in the dataset
    // The output variable must also be compatible with this, now we always keep all
    if (explanations.size() < data.size()) {
        throw std::invalid_argument{"Expansion of lower no. explanations than the dataset size is not supported: "s +
                                    std::to_string(explanations.size()) + " < " + std::to_string(data.size())};
    }

    Print & print = *framework.printPtr;
    bool const printingStats = not print.ignoringStats();
    bool const printingExplanations = not print.ignoringExplanations();
    auto & cexp = print.explanations();

    if (printingStats) { printStatsHead(data); }

    initVerifier();

    // Such incrementality does not seem to be beneficial
    // assertModel();

    Dataset::SampleIndices const indices = makeSampleIndices(data);
    for (auto idx : indices) {
        // Seems quite more efficient than if outside the loop, at least with 'abductive'
        assertModel();

        auto const & output = data.getComputedOutput(idx);
        assertClassification(output);

        auto & verifier = static_cast<xai::verifiers::OpenSMTVerifier const &>(*verifierPtr);
        auto & solver = verifier.getSolver();
        auto & logic = solver.getLogic();
        //- logic.removeAuxVars();
        //- solver.printCurrentAssertionsAsQuery();
        for (opensmt::PTRef phi : solver.getCurrentAssertionsView()) {
            std::cout << logic.printTerm(phi) << std::endl;
        }

        auto & explanationPtr = explanations[idx];
        for (auto & strategy : strategies) {
            strategy->execute(explanationPtr);
        }

        //+ get rid of the conditionals
        auto & explanation = *explanationPtr;
        if (printingStats) { printStats(explanation, data, idx); }
        if (printingExplanations) {
            explanation.print(cexp);
            cexp << std::endl;
        }

        resetClassification();

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

    std::size_t const termSize = explanation.termSize();
    assert(termSize > 0);

    cstats << '\n';
    cstats << "sample [" << idx + 1 << '/' << dataSize << "]: " << sample << '\n';
    cstats << "expected output: " << expClass << '\n';
    cstats << "computed output: " << compClass << '\n';
    cstats << "#checks: " << verifierPtr->getChecksCount() << '\n';
    cstats << "#features: " << expVarSize << '/' << varSize << std::endl;

    assert(not explanation.supportsVolume() or explanation.getRelativeVolumeSkipFixed() > 0);
    assert(explanation.getRelativeVolumeSkipFixed() <= 1);
    assert((explanation.getRelativeVolumeSkipFixed() < 1) == (fixedCount < expVarSize));

    bool const isAbductive = (fixedCount == expVarSize);
    if (isAbductive) {
        assert(termSize == expVarSize);
        assert(explanation.supportsVolume());
        assert(explanation.getRelativeVolumeSkipFixed() == 1);
        return;
    }

    cstats << "#fixed features: " << fixedCount << '/' << varSize << '\n';
    cstats << "#terms: " << termSize << std::endl;

    if (not explanation.supportsVolume()) { return; }

    Float const relVolume = explanation.getRelativeVolumeSkipFixed();

    cstats << "relVolume*: " << std::setprecision(1) << (relVolume * 100) << "%" << std::setprecision(defaultPrecision)
           << std::endl;
}
} // namespace xspace
