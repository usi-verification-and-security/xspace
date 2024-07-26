#include "ITPBasedExplanations.h"

#include <cassert>
#include <unordered_set>

namespace xai::algo {


namespace {
using input_t = std::vector<float>;

}

/*
 * Actual implementation
 */

ITPBasedExplanations::ITPBasedExplanations(std::string networkFile) {
    network = NNet::fromFile(networkFile);
}


void ITPBasedExplanations::setVerifier(std::unique_ptr<InterpolatingVerifier> verifier) {
    this->verifier = std::move(verifier);
}

void ITPBasedExplanations::encodeClassificationConstraint(std::vector<float> const & output, NodeIndex label) {
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
            verifier->addClassificationConstraint(label, 0.0f);
        }
}

ITPBasedExplanations::Result ITPBasedExplanations::computeExplanation(input_t const & inputValues) {
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
        verifier->addLowerBound(0, node, network->getInputLowerBound(node));
        verifier->addUpperBound(0, node, network->getInputUpperBound(node));
    }

    std::unordered_set<NodeIndex> explanationSet;
    std::unordered_set<NodeIndex> freeSet;
    assert(inputSize == inputValues.size());

    encodeClassificationConstraint(output, label);

    while (true) {
        verifier->pushCheckpoint();
        for (NodeIndex node = 0; node < inputSize; ++node) {
            if (not freeSet.contains(node)) {
                verifier->addLowerBound(0, node, inputValues[node]);
                verifier->addUpperBound(0, node, inputValues[node]);
            }
        }
        auto answer = verifier->check();
        if (answer != InterpolatingVerifier::Answer::UNSAT) {
            throw std::logic_error("Query should be unsatisfiable, but isn't!!!");
        }
        auto res = verifier->explain();
        verifier->popCheckpoint();
    }

    std::vector<NodeIndex> explanation{explanationSet.begin(), explanationSet.end()};
    std::sort(explanation.begin(), explanation.end());
    return Result{.explanation = std::move(explanation)};
}

} // namespace xai::algo
