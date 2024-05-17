#include "NNet.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>

namespace stdr = std::ranges;

namespace xai {

namespace{
std::vector<std::string> split(std::string const & s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}
template<typename TTransform>
auto parseValues(std::string const & s, TTransform transformation, char delimiter = ',') {
    auto valueRange = split(s,delimiter) | stdr::views::transform([&](auto const & s) { return transformation(s); });
    using valType = std::invoke_result_t<TTransform, std::string>;
    std::vector<valType> values;
    std::ranges::copy(valueRange, std::back_inserter(values));
    return values;
}

}

/// Load neural network from .nnet file.
/// \param filename the path to the .nnet file
/// \return In-memory representation of the network
/// The file begins with header lines, some information about the network architecture, normalization information, and then model parameters. Line by line:
/// 1: Header text. This can be any number of lines so long as they begin with "//"
/// 2: Four values: Number of layers, number of inputs, number of outputs, and maximum layer size
/// 3: A sequence of values describing the network layer sizes. Begin with the input size, then the size of the first layer, second layer, and so on until the output layer size
/// 4: A flag that is no longer used, can be ignored
/// 5: Minimum values of inputs (used to keep inputs within expected range)
/// 6: Maximum values of inputs (used to keep inputs within expected range)
/// 7: Mean values of inputs and one value for all outputs (used for normalization)
/// 8: Range values of inputs and one value for all outputs (used for normalization)
/// 9+: Begin defining the weight matrix for the first layer, followed by the bias vector. The weights and biases for the second layer follow after, until the weights and biases for the output layer are defined.
std::unique_ptr<NNet> NNet::fromFile(std::string_view filename) {
    if (!std::filesystem::exists(filename)) {
        std::cerr << "File does not exist: " << filename << std::endl;
        return nullptr;
    }
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return nullptr;
    }

    std::string line;

    // Skip header lines
    while (std::getline(file, line)) {
        if (line.find("//") == std::string::npos) {
            break;
        }
    }

    std::vector<std::string> networkArchitecture = split(line, ',');
    assert(networkArchitecture.size() == 4);
    std::size_t numLayers = std::stoull(networkArchitecture[0]) + 1; // The format does not include input layer, but we do
    std::size_t numInputs = std::stoull(networkArchitecture[1]);
    std::size_t numOutputs = std::stoull(networkArchitecture[2]);
    std::size_t maxLayerSize = std::stoull(networkArchitecture[3]);

    // Specify layer sizes
    std::getline(file, line);
    auto layer_sizes = parseValues(line, [](auto const & val) { return std::stoull(val); });
    assert(layer_sizes.size() == numLayers);

    // Flag that can be ignored
    std::getline(file, line);

    // Min and max input values
    auto transformation = [](auto const & val) { return std::stof(val); };
    std::getline(file, line);
    std::vector<float> inputMinValues = parseValues(line, transformation);
    std::getline(file, line);
    std::vector<float> inputMaxValues = parseValues(line, transformation);
    assert(inputMinValues.size() == numInputs and inputMaxValues.size() == numInputs);

    // Skip normalization information
    for (int i = 0; i < 2; i++) {
        std::getline(file, line);
    }

    // Parse model parameters
    weights_t weights(numLayers);
    std::vector<std::vector<float>> biases(numLayers);
    for (int layer = 0; layer < numLayers - 1; layer++) {
        // Parse weights
        auto layerSize = layer_sizes.at(layer + 1);
        for (int i = 0; i < layerSize; i++) {
            std::getline(file, line);
            std::vector<std::string> weightStrings = split(line, ',');
            weights[layer].emplace_back();
            for (const auto& weightString : weightStrings) {
                weights[layer][i].push_back(std::stof(weightString));
            }
        }

        // Parse biases
        for (int i = 0; i < layerSize; i++) {
            std::getline(file, line);
            std::vector<std::string> biasStrings = split(line, ',');
            std::string biasString = biasStrings[0];
            biases[layer].push_back(std::stof(biasString));
        }
    }
    file.close();
    auto network = std::unique_ptr<NNet>(new NNet());
    network->numInputs = numInputs;
    network->numOutputs = numOutputs;
    network->numLayers = numLayers;
    network->maxLayerSize = maxLayerSize;
    network->inputMinimums = std::move(inputMinValues);
    network->inputMaximums = std::move(inputMaxValues);
    network->weights = std::move(weights);
    network->biases = std::move(biases);
    return network;
}

std::size_t NNet::getInputSize() const {
    return numInputs;
}

std::size_t NNet::getLayerSize(std::size_t layerNum) const {
    if (layerNum == 0)
        return numInputs;
    assert(layerNum <= biases.size());
    return biases[layerNum - 1].size();
}

std::vector<float> const & NNet::getWeights(std::size_t layerNum, std::size_t nodeIndex) const {
    assert(layerNum > 0);
    assert(nodeIndex < getLayerSize(layerNum));
    return weights[layerNum - 1][nodeIndex];
}

float NNet::getBias(std::size_t layerNum, std::size_t nodeIndex) const {
    assert(layerNum > 0);
    assert(nodeIndex < getLayerSize(layerNum));
    return biases[layerNum - 1][nodeIndex];
}

float NNet::getInputLowerBound(std::size_t nodeIndex) const {
    assert(nodeIndex < getLayerSize(0));
    return inputMinimums.at(nodeIndex);
}

float NNet::getInputUpperBound(std::size_t nodeIndex) const {
    assert(nodeIndex < getLayerSize(0));
    return inputMaximums.at(nodeIndex);
}

} // namespace xai