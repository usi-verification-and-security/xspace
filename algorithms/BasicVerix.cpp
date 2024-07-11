#include "BasicVerix.h"

#include <cassert>
#include <iostream>
#include <numeric>
#include <unordered_set>

namespace xai::algo {


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
                                                  std::vector<std::size_t> featureOrder) {
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
    // TODO: Come up with heuristic for feature ordering
    if (featureOrder.empty()) {
        for (std::size_t node = 0; node < inputSize; ++node) {
            featureOrder.push_back(node);
        }
    } else {
        assert(featureOrder.size() == inputSize);
    }
    for (NodeIndex nodeToConsider : featureOrder) {
        for (NodeIndex node = 0; node < inputSize; ++node) {
            if (node == nodeToConsider or freeSet.contains(node)) { // this input feature is free
                verifier->addLowerBound(0, node, inputLowerBounds[node]);
                verifier->addUpperBound(0, node, inputUpperBounds[node]);
            } else { // this input feature is fixed
                verifier->addLowerBound(0, node, inputValues[node]);
                verifier->addUpperBound(0, node, inputValues[node]);
            }
        }
        // TODO: General property specification?
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
                                                                 std::vector<std::size_t> featureOrder) {
    auto output = computeOutput(inputValues, *network);
    NodeIndex label = std::max_element(output.begin(), output.end()) - output.begin();
    float freedomFactor = 1.0f;
    auto result = computeExplanation(inputValues, freedomFactor, featureOrder);
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

} // namespace xai::algo
