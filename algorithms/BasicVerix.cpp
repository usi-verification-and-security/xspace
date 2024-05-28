#include "BasicVerix.h"

#include <cassert>
#include <iostream>
#include <numeric>
#include <unordered_set>

namespace xai::algo {


namespace {
using input_t = std::vector<float>;
auto computeOutput(input_t const & inputValues, NNet const & network) {
    auto inputSize = network.getInputSize();
    if (inputValues.size() != inputSize) { throw std::logic_error("Input values do not have expected size!"); }

    auto previousLayerValues = inputValues;
    std::vector<float> currentLayerValues;
    for (LayerIndex layer = 1; layer < network.getNumLayers(); ++layer) {
        for (NodeIndex node = 0; node < network.getLayerSize(layer); ++node) {
            auto const & incomingWeights = network.getWeights(layer, node);
            assert(incomingWeights.size() == previousLayerValues.size());
            std::vector<float> addends;
            for (std::size_t i = 0; i < incomingWeights.size(); ++i) {
                addends.push_back(incomingWeights[i] * previousLayerValues[i]);
            }
            currentLayerValues.push_back(std::accumulate(addends.begin(), addends.end(), network.getBias(layer, node)));
        }
        if (layer < network.getNumLayers() - 1) {
            std::transform(currentLayerValues.begin(), currentLayerValues.end(), currentLayerValues.begin(), [](float val) {
               return val >= 0.0f ? val : 0.0f;
            });
            previousLayerValues = std::move(currentLayerValues);
            currentLayerValues.clear();
        }
    }
    return currentLayerValues;
}

}

/*
 * Actual implementation
 */

BasicVerix::BasicVerix(std::string networkFile) : networkFile(networkFile) {}


void BasicVerix::setVerifier(std::unique_ptr<Verifier> verifier) {
    this->verifier = std::move(verifier);
}

BasicVerix::Result BasicVerix::computeExplanation(input_t const & inputValues, float freedom_factor) {
    assert(freedom_factor <= 1.0 and freedom_factor >= 0.0);
    auto network = NNet::fromFile(networkFile);
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
    for (NodeIndex nodeToConsider = 0; nodeToConsider < inputSize; ++nodeToConsider) {
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
        auto outputLayerIndex = network->getNumLayers() - 1;
        if (output.size() == 1) {
            // With single output, the flip in classification means flipping the value across certain threshold
            constexpr float THRESHOLD = 0;
            constexpr float PRECISION = 0.0625f;
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
            for (NodeIndex outputNode = 0; outputNode < outputLayerSize; ++outputNode) {
                if (outputNode == label) { continue; }
                throw std::logic_error("Not implemented yet!");
            }
        }
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

} // namespace xai::algo
