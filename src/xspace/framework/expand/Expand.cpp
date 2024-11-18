#include "Expand.h"

#include "../Config.h"
#include "strategy/Strategy.h"

#include <xspace/common/String.h>
#include <xspace/explanation/Explanation.h>
#include <xspace/nn/Dataset.h>

#include <verifiers/Verifier.h>
#include <verifiers/opensmt/OpenSMTVerifier.h>
#ifdef MARABOU
#include <verifiers/marabou/MarabouVerifier.h>
#endif

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>

namespace xspace {
std::unique_ptr<xai::verifiers::Verifier> Framework::Expand::makeVerifier(std::string_view name) {
    if (tolower(name) == "opensmt") {
        return std::make_unique<xai::verifiers::OpenSMTVerifier>();
#ifdef MARABOU
    } else if (tolower(name) == "marabou") {
        return std::make_unique<xai::verifiers::MarabouVerifier>();
#endif
    } else {
        // !!!
        //- throw std::runtime_error(std::cerr << "Unrecognized verifier name: " << name << '\n';)
        return {};
    }
}

void Framework::Expand::setStrategies(std::istream & is) {
    std::string line;
    while (std::getline(is, line, ',')) {
        std::istringstream issLine{std::move(line)};
        std::string name;
        issLine >> name;
        std::vector<std::string> params;
        for (std::string param; issLine >> param;) {
            params.push_back(std::move(param));
        }

        auto const nameLower = tolower(name);
        if (nameLower == "abductive") {
            assert(params.empty());
            addStrategy(std::make_unique<AbductiveStrategy>(*this));
            continue;
        }

        //- throw std::runtime_error(std::cerr << "Unrecognized strategy name: " << name << '\n';)
        assert(false);
    }
}

void Framework::Expand::operator()(std::vector<IntervalExplanation> & explanations, Dataset const & data) {
    assert(verifierPtr);
    assert(not strategies.empty());
    assert(not outputs.empty());

    assertModel();

    bool const verbose = framework.getConfig().isVerbose();
    auto const defaultPrecision = std::cout.precision();

    std::size_t const size = explanations.size();
    assert(outputs.size() == size);
    for (std::size_t i = 0; i < size; ++i) {
        verifierPtr->push();

        auto const & output = outputs[i];
        assertClassification(output);

        auto & explanation = explanations[i];
        for (auto & strategy : strategies) {
            strategy->execute(explanation);
        }

        verifierPtr->pop();
        verifierPtr->resetSample();

        if (not verbose) { continue; }

        std::size_t const varSize = framework.varSize();
        std::size_t const expSize = explanation.size();
        assert(expSize <= varSize);

        auto const & samples = data.getSamples();
        auto const & expOutputs = data.getOutputs();
        assert(samples.size() == expOutputs.size());
        assert(outputs.size() == expOutputs.size());
        auto const & sample = samples[i];
        auto const & expOutput = expOutputs[i];

        std::size_t const fixedCount = explanation.getFixedCount();
        assert(fixedCount <= expSize);

        std::cout << "sample: " << sample << '\n';
        std::cout << "expected output: " << expOutput << '\n';
        std::cout << "computed output: " << output.classifiedIdx << '\n';
        std::cout << "explanation size: " << expSize << '/' << varSize << std::endl;

        assert(explanation.getRelativeVolumeSkipFixed() > 0);
        assert(explanation.getRelativeVolumeSkipFixed() <= 1);
        assert((explanation.getRelativeVolumeSkipFixed() < 1) == (fixedCount < expSize));
        if (fixedCount < expSize) {
            Float const relVolume = explanation.getRelativeVolumeSkipFixed();

            std::cout << "fixed features: " << fixedCount << '/' << varSize << std::endl;
            std::cout << "relVolume*: " << std::setprecision(1) << (relVolume * 100) << "%"
                      << std::setprecision(defaultPrecision) << std::endl;
        }

        std::cerr << explanation << std::endl;
    }
}

void Framework::Expand::assertModel() {
    auto & nn = framework.getNetwork();
    verifierPtr->loadModel(nn);
}

void Framework::Expand::assertClassification(Output const & output) {
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

    auto const outputLayerSize = network.getLayerSize(outputLayerIndex);
    assert(outputLayerSize == outputValues.size());
    verifierPtr->addClassificationConstraint(output.classifiedIdx, 0);
}

void Framework::Expand::assertBound(VarIdx idx, Bound const & bnd) {
    Float const val = bnd.getValue();
    if (bnd.isEq()) {
        verifierPtr->addEquality(0, idx, val);
    } else if (bnd.isLower()) {
        verifierPtr->addLowerBound(0, idx, val);
    } else {
        assert(bnd.isUpper());
        verifierPtr->addUpperBound(0, idx, val);
    }
}

bool Framework::Expand::checkFormsExplanation() {
    auto answer = verifierPtr->check();
    assert(answer == xai::verifiers::Verifier::Answer::SAT or answer == xai::verifiers::Verifier::Answer::UNSAT);
    return (answer == xai::verifiers::Verifier::Answer::UNSAT);
}
} // namespace xspace
