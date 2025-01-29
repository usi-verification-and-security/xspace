#ifndef XAI_SMT_NNET_H
#define XAI_SMT_NNET_H

#include <memory>
#include <string>
#include <vector>

namespace xai::nn {
using Float = double;

class NNet {
public:
    using input_t = std::vector<Float>;
    using output_t = std::vector<Float>;

    static std::unique_ptr<NNet> fromFile(std::string_view filename);

    std::size_t getNumLayers() const { return numLayers; }

    std::size_t getLayerSize(std::size_t layerNum) const;
    std::size_t getInputSize() const;

    std::vector<Float> const & getWeights(std::size_t layerNum, std::size_t nodeIndex) const;

    Float getBias(std::size_t layerNum, std::size_t nodeIndex) const;

    Float getInputLowerBound(std::size_t node) const;
    Float getInputUpperBound(std::size_t node) const;

private:
    NNet() = default;

    using weights_t = std::vector<std::vector<std::vector<Float>>>;
    using biases_t = std::vector<std::vector<Float>>;

    std::size_t numLayers;
    std::size_t numInputs;
    std::size_t numOutputs;
    std::size_t maxLayerSize;
    std::vector<Float> inputMinimums;
    std::vector<Float> inputMaximums;
    weights_t weights;
    biases_t biases;
};

NNet::output_t computeOutput(NNet::input_t const &, NNet const &);

}



#endif //XAI_SMT_NNET_H
