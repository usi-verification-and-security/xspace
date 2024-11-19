#include "HypothesisTesting.h"

#include "verifiers/marabou/MarabouVerifier.h"
#include "NNet.h"

#include <cassert>
#include <iostream>
#include <string>

namespace xai {

void hypothesisHighRisk() {
    std::string networkPath = "models/heartAttack.nnet";
    auto network = nn::NNet::fromFile(networkPath);
    auto input = std::vector<float>{39.0f, 0.0f, 2.0f,94.0f,199.0f,0.0f,1.0f,179.0f,0.0f,0.0f,2.0f,0.0f,2.0f};
    auto outputs = computeOutput(input, *network);
    assert(outputs.size() == 1);
    auto output = outputs.at(0);
    std::cout << output << std::endl;

    auto inputSize = network->getInputSize();
    assert(inputSize == input.size());
    auto verifier = std::make_unique<verifiers::MarabouVerifier>();
    verifier->init();
    verifier->loadModel(*network);
    verifier->push();
//    std::vector<NodeIndex> fixedIndices = {3,7,8,9,11};
    std::vector<NodeIndex> fixedLowerBounds = {7};
    std::vector<NodeIndex> fixedUpperBounds = {3,8,9,11};
    for (auto index : fixedLowerBounds) {
        verifier->addLowerBound(0, index, input[index]);
    }
    for (auto index : fixedUpperBounds) {
        verifier->addUpperBound(0, index, input[index]);
    }
    // Property: output will be negative
    constexpr float PRECISION = 0.0625f;
    verifier->addUpperBound(network->getNumLayers() - 1, 0, -PRECISION);
    auto res = verifier->check();
    switch (res) {
        case Verifier::Answer::SAT:
            std::cout << "Output CAN be negative!\n";
            break;
        case Verifier::Answer::UNSAT:
            std::cout << "Output CANNOT be negative!\n";
            break;
        default:
            std::cout << "Unexpected situation!\n";
            break;
    }
    std::cout << std::flush;
}

void hypothesisLowRisk() {
    std::string networkPath = "models/heartAttack.nnet";
    auto network = nn::NNet::fromFile(networkPath);
//    auto input = std::vector<float>{67.0f, 1.0f, 0.0f, 120.0f, 237.0f, 0.0f, 1.0f, 71.0f, 0.0f, 1.0f, 1.0f, 0.0f, 2.0f};
    auto input = std::vector<float>{67.0f, 1.0f, 0.0f, 120.0f, 237.0f, 0.0f, 1.0f, 71.0f, 0.0f, 1.0f, 1.0f, 0.0f, 2.0f};
    auto outputs = computeOutput(input, *network);
    assert(outputs.size() == 1);
    auto output = outputs.at(0);
    std::cout << output << std::endl;

    auto inputSize = network->getInputSize();
    assert(inputSize == input.size());
    auto verifier = std::make_unique<verifiers::MarabouVerifier>();
    verifier->init();
    verifier->loadModel(*network);
    verifier->push();
//    std::vector<NodeIndex> fixedIndices = {4,7};
    std::vector<NodeIndex> fixedLowerBounds = {4};
    std::vector<NodeIndex> fixedUpperBounds = {7};
    for (auto index : fixedLowerBounds) {
        verifier->addLowerBound(0, index, input[index]);
    }
    for (auto index : fixedUpperBounds) {
        verifier->addUpperBound(0, index, input[index]);
    }
    // Property: output will be negative
    constexpr float PRECISION = 0.0625f;
    verifier->addLowerBound(network->getNumLayers() - 1, 0, PRECISION);
    auto res = verifier->check();
    switch (res) {
        case Verifier::Answer::SAT:
            std::cout << "Output CAN be positive!\n";
            break;
        case Verifier::Answer::UNSAT:
            std::cout << "Output CANNOT be positive!\n";
            break;
        default:
            std::cout << "Unexpected situation!\n";
            break;
    }
    std::cout << std::flush;
}

void hypothesisLowRiskGeneralized() {
    std::string networkPath = "models/heartAttack.nnet";
    auto network = nn::NNet::fromFile(networkPath);
    auto input = std::vector<float>{67.0f, 1.0f, 0.0f, 120.0f, 237.0f, 0.0f, 1.0f, 71.0f, 0.0f, 1.0f, 1.0f, 0.0f, 2.0f};
    auto outputs = computeOutput(input, *network);
    assert(outputs.size() == 1);
    auto output = outputs.at(0);
    std::cout << output << std::endl;

    auto inputSize = network->getInputSize();
    assert(inputSize == input.size());
    auto verifier = std::make_unique<verifiers::MarabouVerifier>();
    verifier->init();
    verifier->loadModel(*network);
    verifier->push();
    using bound_t = std::pair<NodeIndex, float>;
//    std::vector<NodeIndex> fixedLowerBounds = {4};
//    std::vector<NodeIndex> fixedUpperBounds = {7};
    std::vector<bound_t> fixedLowerBounds = {{4,235}};
    std::vector<bound_t> fixedUpperBounds = {{7, 75}};
    for (auto [index, value] : fixedLowerBounds) {
        verifier->addLowerBound(0, index, value);
    }
    for (auto [index, value] : fixedUpperBounds) {
        verifier->addUpperBound(0, index, value);
    }
    // Property: output will be negative
//    constexpr float PRECISION = 0.0625f;
    constexpr float PRECISION = 0.0f;
    verifier->addLowerBound(network->getNumLayers() - 1, 0, PRECISION);
    auto res = verifier->check();
    switch (res) {
        case Verifier::Answer::SAT:
            std::cout << "Output CAN be positive!\n";
            break;
        case Verifier::Answer::UNSAT:
            std::cout << "Output CANNOT be positive!\n";
            break;
        default:
            std::cout << "Unexpected situation!\n";
            break;
    }
    std::cout << std::flush;
}

void HypothesisTesting::run() {
    hypothesisLowRiskGeneralized();
}

} // namespace xai
