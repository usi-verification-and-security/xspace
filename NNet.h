#ifndef XAI_SMT_NNET_H
#define XAI_SMT_NNET_H

#include <memory>
#include <string>
#include <vector>

namespace xai {
class NNet {
public:
    static std::unique_ptr<NNet> fromFile(std::string_view filename);

    std::size_t getNumLayers() const { return numLayers; }

    std::size_t getLayerSize(std::size_t layerNum) const;
    std::size_t getInputSize() const;

    std::vector<float> const & getWeights(std::size_t layerNum, std::size_t nodeIndex) const;

    float getBias(std::size_t layerNum, std::size_t nodeIndex) const;

    float getInputLowerBound(std::size_t node) const;
    float getInputUpperBound(std::size_t node) const;

private:
    NNet() = default;

    using weights_t = std::vector<std::vector<std::vector<float>>>;
    using biases_t = std::vector<std::vector<float>>;

    std::size_t numLayers;
    std::size_t numInputs;
    std::size_t numOutputs;
    std::size_t maxLayerSize;
    std::vector<float> inputMinimums;
    std::vector<float> inputMaximums;
    weights_t weights;
    biases_t biases;


};

}



#endif //XAI_SMT_NNET_H
