#include "BasicVerix.h"

#include "experiments/Config.h"
#include "experiments/Utils.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <unordered_set>
#include <algorithm>

#include "verifiers/opensmt/OpenSMTVerifier.h"
#include "MainSolver.h"

namespace xai::algo {

using namespace opensmt;

using experiments::Config;

namespace {
using input_t = std::vector<float>;

}

/*
 * Actual implementation
 */

BasicVerix::BasicVerix(std::string networkFile) {
    network = NNet::fromFile(networkFile);
}


void BasicVerix::setVerifier(std::unique_ptr<Verifier> verifier) {
    this->verifier = std::move(verifier);
}

void BasicVerix::encodeClassificationConstraint(std::vector<float> const & output, NodeIndex label) {
    auto outputLayerIndex = network->getNumLayers() - 1;
        if (output.size() == 1) {
            // With single output, the flip in classification means flipping the value across certain threshold
            constexpr float THRESHOLD = 0;
            constexpr float PRECISION = 0.015625f;
            float outputValue = output[0];
            if (outputValue >= THRESHOLD) {
                verifier->addUpperBound(outputLayerIndex, 0, THRESHOLD - PRECISION);
            } else {
                verifier->addLowerBound(outputLayerIndex, 0, THRESHOLD + PRECISION);
            }
        } else {
            auto outputLayerSize = network->getLayerSize(outputLayerIndex);
            assert(outputLayerSize == output.size());
            // TODO: Build disjunction of inequalities
            verifier->addClassificationConstraint(label, 0);
//            for (NodeIndex outputNode = 0; outputNode < outputLayerSize; ++outputNode) {
//                if (outputNode == label) { continue; }
//                throw std::logic_error("Not implemented yet!");
//            }
        }
}

BasicVerix::Result BasicVerix::computeExplanation(input_t const & inputValues, float freedom_factor,
                                                  std::vector<std::size_t> const & featureOrder) {
    assert(freedom_factor <= 1.0 and freedom_factor >= 0.0);
    if (not network or not verifier)
        return Result{};

    verifier->loadModel(*network);

    auto output = computeOutput(inputValues, *network);
    NodeIndex label = std::max_element(output.begin(), output.end()) - output.begin();
    auto inputSize = network->getLayerSize(0);

    // Compute lower and upper bounds based on the freedom factor
    std::vector<float> inputLowerBounds;
    std::vector<float> inputUpperBounds;
    for (NodeIndex node = 0; node < inputSize; ++node) {
        float input_freedom = (network->getInputUpperBound(node) - network->getInputLowerBound(node)) * freedom_factor;
        inputLowerBounds.push_back(std::max(inputValues[node] - input_freedom, network->getInputLowerBound(node)));
        inputUpperBounds.push_back(std::min(inputValues[node] + input_freedom, network->getInputUpperBound(node)));
    }

    std::unordered_set<NodeIndex> explanationSet;
    std::unordered_set<NodeIndex> freeSet;
    assert(inputSize == inputValues.size());
    assert(featureOrder.size() == inputSize);
    for (NodeIndex nodeToConsider : featureOrder) {
        for (NodeIndex node = 0; node < inputSize; ++node) {
            if (node == nodeToConsider or freeSet.contains(node)) { // this input feature is free
                verifier->addLowerBound(0, node, inputLowerBounds[node]);
                verifier->addUpperBound(0, node, inputUpperBounds[node]);
            } else { // this input feature is fixed
                verifier->addEquality(0, node, inputValues[node]);
            }
        }
        // TODO: General property specification?
        // TODO: Figure out how to do this only once!
        encodeClassificationConstraint(output, label);
        auto answer = verifier->check();
        if (answer == Verifier::Answer::UNSAT) {
            freeSet.insert(nodeToConsider);
        } else {
            explanationSet.insert(nodeToConsider);
        }
        verifier->clearAdditionalConstraints();
    }
    std::vector<NodeIndex> explanation{explanationSet.begin(), explanationSet.end()};
    std::sort(explanation.begin(), explanation.end());
    return Result{.explanation = std::move(explanation)};
}

BasicVerix::GeneralizedExplanation BasicVerix::computeGeneralizedExplanation(const std::vector<float> &inputValues,
                                       std::vector<std::size_t> const & featureOrder, int threshold) {
    auto output = computeOutput(inputValues, *network);
    NodeIndex label = std::max_element(output.begin(), output.end()) - output.begin();
    float freedomFactor = 1.0f;
    auto result = computeExplanation(inputValues, freedomFactor, featureOrder);

    // Algorithm 1 in section 5.2 in the paper
    auto const & explanationFeatures = result.explanation;
    struct Bound { NodeIndex index; float value; };
    std::vector<Bound> lowerBounds;
    std::vector<Bound> upperBounds;
    for (auto index : explanationFeatures) {
        lowerBounds.push_back({.index = index, .value = inputValues.at(index)});
        upperBounds.push_back({.index = index, .value = inputValues.at(index)});
    }
    auto check = [&](){
        for (auto lb: lowerBounds) {
            verifier->addLowerBound(0, lb.index, lb.value);
        }
        for (auto ub: upperBounds) {
            verifier->addUpperBound(0, ub.index, ub.value);
        }
        // TODO: Figure out how to do this only once!
        this->encodeClassificationConstraint(output, label);
        auto answer = verifier->check();
        verifier->clearAdditionalConstraints();
        return answer;
    };

    for (NodeIndex inputIndex : explanationFeatures) {

        float inputValue = inputValues.at(inputIndex);
        float lowerBound = network->getInputLowerBound(inputIndex);
        float upperBound = network->getInputUpperBound(inputIndex);

        if (lowerBound < inputValue) {
            // Try relaxing lower bound
            auto it = std::find_if(lowerBounds.begin(), lowerBounds.end(), [&](auto const & lb) { return lb.index == inputIndex; });
            it->value = lowerBound;
            auto answer = check();
            if (answer == Verifier::Answer::UNSAT) {
                lowerBounds.erase(it);
            } else {

                auto tmp = lowerBound;
                for (int i = 0 ; i < threshold; i++)
                {
                    tmp = (inputValue + tmp)/2;
                    it->value = tmp;
                    answer = check();
                    if (answer == Verifier::Answer::UNSAT) {
                        it->value = tmp;
                        break;
                    }
                }

                if (answer != Verifier::Answer::UNSAT)
                    it->value = inputValue;
            }
        }

        if (upperBound > inputValue) {
            // Try relaxing upper bound
            auto it = std::find_if(upperBounds.begin(), upperBounds.end(), [&](auto const & ub) { return ub.index == inputIndex; });
            it->value = upperBound;
            auto answer = check();
            if (answer == Verifier::Answer::UNSAT) {
                upperBounds.erase(it);
            } else {

                auto tmp = inputValue;
                for (int i = 0 ; i < threshold; i++)
                {
                    tmp = (upperBound + tmp)/2;
                    it->value = tmp;
                    answer = check();
                    if (answer == Verifier::Answer::UNSAT) {
                        it->value = tmp;
                        break;
                    }
                }

              if (answer != Verifier::Answer::UNSAT)
                  it->value = inputValue;
            }
        }
    }

    GeneralizedExplanation generalizedExplanation;
    auto & constraints = generalizedExplanation.constraints;
    for (auto const & lb : lowerBounds) {
        constraints.push_back({.inputIndex = lb.index, .boundType = GeneralizedExplanation::BoundType::LB, .value = lb.value});
    }
    for (auto const & ub : upperBounds) {
        auto it = std::find_if(constraints.begin(), constraints.end(), [&](auto const & bound){ return bound.inputIndex == ub.index; });
        if (it != constraints.end() and it->value == ub.value) {
            it->boundType = GeneralizedExplanation::BoundType::EQ;
        } else {
            constraints.push_back({.inputIndex = ub.index, .boundType = GeneralizedExplanation::BoundType::UB, .value = ub.value});
        }
    }
    std::sort(constraints.begin(), constraints.end(), [](auto const & first, auto const & second) {
        return first.inputIndex < second.inputIndex or (first.inputIndex == second.inputIndex and first.boundType == GeneralizedExplanation::BoundType::LB and second.boundType != GeneralizedExplanation::BoundType::LB);
    });
    return generalizedExplanation;
}

BasicVerix::Result BasicVerix::computeOpenSMTExplanation(input_t const & inputValues, float freedom_factor,
                                                         std::vector<std::size_t> const & featureOrder) {
    assert(freedom_factor <= 1.0 and freedom_factor >= 0.0);
    if (not network or not verifier)
        return Result{};

    verifier->loadModel(*network);

    auto output = computeOutput(inputValues, *network);
    NodeIndex label = std::max_element(output.begin(), output.end()) - output.begin();
    auto inputSize = network->getLayerSize(0);

    // Compute lower and upper bounds based on the freedom factor
    std::vector<float> inputLowerBounds;
    std::vector<float> inputUpperBounds;
    for (NodeIndex node = 0; node < inputSize; ++node) {
        float input_freedom = (network->getInputUpperBound(node) - network->getInputLowerBound(node)) * freedom_factor;
        inputLowerBounds.push_back(std::max(inputValues[node] - input_freedom, network->getInputLowerBound(node)));
        inputUpperBounds.push_back(std::min(inputValues[node] + input_freedom, network->getInputUpperBound(node)));
    }

    assert(inputSize == inputValues.size());
    assert(featureOrder.size() == inputSize);
    for (NodeIndex node = 0; node < inputSize; ++node) {
        verifier->addLowerBound(0, node, inputLowerBounds[node]);
        verifier->addUpperBound(0, node, inputUpperBounds[node]);
    }
    encodeClassificationConstraint(output, label);

    assert(dynamic_cast<verifiers::OpenSMTVerifier *>(verifier.get()));
    auto & openSMT = static_cast<verifiers::OpenSMTVerifier &>(*verifier);
    auto & solver = openSMT.getSolver();

    // to get psi:
    // solver.printFramesAsQuery();

    if constexpr (Config::interpolation) {
        solver.push();
    }
    for (NodeIndex node : featureOrder) {
        auto const val = inputValues[node];
        assert(val >= inputLowerBounds[node]);
        assert(val <= inputUpperBounds[node]);

        if constexpr (Config::samplesOnly) {
            verifier->addEquality(0, node, val, true);
            continue;
        }

        bool const isLower = (val == inputLowerBounds[node]);
        bool const isUpper = (val == inputUpperBounds[node]);
        // It is worthless to assert bounds that already correspond to the bounds of the domain
        if (not isLower) { verifier->addLowerBound(0, node, val, true); }
        if (not isUpper) { verifier->addUpperBound(0, node, val, true); }
    }
    auto answer = verifier->check();
    assert(answer == Verifier::Answer::UNSAT);

    auto unsatCore = solver.getUnsatCore();

    if constexpr (Config::interpolation) {
        solver.pop();

        auto const firstIdx = solver.getInsertedFormulasCount();
        for (PTRef term : unsatCore->getNamedTerms()) {
            solver.insertFormula(term);
        }
        auto const lastIdx = solver.getInsertedFormulasCount()-1;

        auto answer = verifier->check();
        assert(answer == Verifier::Answer::UNSAT);

        ipartitions_t part = 0;
        for (auto idx = firstIdx; idx <= lastIdx; ++idx) {
            setbit(part, idx);
        }

        vec<PTRef> itps;
        auto interpolationContext = solver.getInterpolationContext();
        interpolationContext->getSingleInterpolant(itps, part);
        assert(itps.size() == 1);
        PTRef itp = itps[0];

        std::cerr << experiments::fixOpenSMTString(solver.getLogic().pp(itp)) << std::endl << std::endl;
    } else {
        auto & logic = solver.getLogic();
        PTRef ucore = logic.mkAnd(unsatCore->getNamedTerms());
        std::cerr << experiments::fixOpenSMTString(logic.pp(ucore)) << std::endl << std::endl;
    }

    // to get the whole query:
    // solver.printFramesAsQuery();

    if constexpr (Config::samplesOnly) {
        std::vector<NodeIndex> explanation;
        for (PTRef term : unsatCore->getNamedTerms()) {
            assert(openSMT.containsInputEquality(term));
            explanation.push_back(openSMT.nodeIndexOfInputEquality(term));
        }

        std::sort(explanation.begin(), explanation.end());
        return Result{.explanation = std::move(explanation)};
    }

    std::vector<NodeIndex> explanationLower;
    std::vector<NodeIndex> explanationUpper;
    std::unordered_set<NodeIndex> explanationLowerSet;
    std::unordered_set<NodeIndex> explanationUpperSet;
    auto insertCoreTerm = [&](auto isLower, PTRef term){
        NodeIndex node = [&]{
            if constexpr (isLower) { return openSMT.nodeIndexOfInputLowerBound(term); }
            else { return openSMT.nodeIndexOfInputUpperBound(term); }
        }();
        auto & boundExplanation = [&]() -> auto & {
            if constexpr (isLower) { return explanationLower; }
            else { return explanationUpper; }
        }();
        auto & boundExplanationSet = [&]() -> auto & {
            if constexpr (isLower) { return explanationLowerSet; }
            else { return explanationUpperSet; }
        }();
        boundExplanation.push_back(node);
        auto const [_, inserted] = boundExplanationSet.insert(node);
        assert(inserted);
    };
    for (PTRef term : unsatCore->getNamedTerms()) {
        bool const containsLower = openSMT.containsInputLowerBound(term);
        if (containsLower) {
            insertCoreTerm(std::true_type(), term);
            continue;
        }
        bool const containsUpper = openSMT.containsInputUpperBound(term);
        if (containsUpper) {
            insertCoreTerm(std::false_type(), term);
            continue;
        }

        assert(false);
    }

    double relVolume = 1;
    auto processCoreTerm = [&](auto isLower, NodeIndex node){
        auto & otherBoundExplanation = [&]() -> auto & {
            if constexpr (isLower) { return explanationUpper; }
            else { return explanationLower; }
        }();
        auto & otherBoundExplanationSet = [&]() -> auto & {
            if constexpr (isLower) { return explanationUpperSet; }
            else { return explanationLowerSet; }
        }();

        auto const val = inputValues[node];
        auto const lo = inputLowerBounds[node];
        auto const hi = inputUpperBounds[node];
        double const size = hi - lo;
        double const boundedSize = [=]{
            if constexpr (isLower) { return hi - val; }
            else { return val - lo; }
        }();
        assert(boundedSize >= 0);
        assert(boundedSize <= size);
        assert(boundedSize != size or (not isLower and otherBoundExplanationSet.contains(node)));

        if (otherBoundExplanationSet.contains(node)) {
            return;
        }

        if (boundedSize == 0) {
            otherBoundExplanation.push_back(node);
            return;
        }

        relVolume *= boundedSize/size;
    };
    std::for_each(explanationLower.begin(), explanationLower.end(), [&](NodeIndex node){ processCoreTerm(std::true_type(), node); });
    std::for_each(explanationUpper.begin(), explanationUpper.end(), [&](NodeIndex node){ processCoreTerm(std::false_type(), node); });

    std::sort(explanationLower.begin(), explanationLower.end());
    std::sort(explanationUpper.begin(), explanationUpper.end());
    std::vector<NodeIndex> explanationEqual;
    std::set_intersection(explanationLower.begin(), explanationLower.end(),
                          explanationUpper.begin(), explanationUpper.end(),
                          std::inserter(explanationEqual, explanationEqual.begin()));
    std::vector<NodeIndex> explanation;
    std::set_union(explanationLower.begin(), explanationLower.end(),
                   explanationUpper.begin(), explanationUpper.end(),
                   std::inserter(explanation, explanation.begin()));

    auto const defaultPrecision = std::cout.precision();
    std::cout << "fixed features: " << explanationEqual.size() << "/" << inputSize << std::endl;
    std::cout << "relVolume*: " << std::setprecision(1) << (relVolume*100) << "%" << std::setprecision(defaultPrecision) << std::endl;
    return Result{.explanation = std::move(explanation)};
}

} // namespace xai::algo
