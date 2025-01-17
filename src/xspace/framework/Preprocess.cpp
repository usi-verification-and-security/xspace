#include "Preprocess.h"

#include "explanation/IntervalExplanation.h"
#include "explanation/VarBound.h"

#include <xspace/common/Macro.h>

#include <nn/NNet.h>

#include <algorithm>
#include <cassert>
#include <concepts>

namespace xspace {
Framework::Preprocess::Preprocess(Framework & fw, Dataset & data) : framework{fw}, dataset{data} {
    assert(not framework.varNames.empty());

    initDataset();
}

void Framework::Preprocess::initDataset() {
    auto const & samples = dataset.getSamples();
    std::size_t const size = dataset.size();
    assert(size == samples.size());
    assert(not samples.empty());
    Dataset::Outputs outputs;
    outputs.reserve(size);
    for (auto const & sample : samples) {
        assert(sample.size() == framework.varSize());
        Dataset::Output output = computeOutput(sample);
        outputs.push_back(std::move(output));
    }

    assert(outputs.size() == size);
    dataset.setComputedOutputs(std::move(outputs));
}

Explanations Framework::Preprocess::makeExplanationsFromSamples() const {
    auto const & samples = dataset.getSamples();
    std::size_t const size = dataset.size();
    assert(size == samples.size());
    Explanations explanations;
    explanations.reserve(size);
    std::size_t const vSize = framework.varSize();
    for (auto const & sample : samples) {
        assert(sample.size() == vSize);

        IntervalExplanation iexplanation{framework};
        for (VarIdx idx = 0; idx < vSize; ++idx) {
            Float val = sample[idx];
            iexplanation.insertVarBound(VarBound{framework, idx, val});
        }
        explanations.push_back(MAKE_UNIQUE(std::move(iexplanation)));
    }

    assert(explanations.size() == size);
    return explanations;
}

Dataset::Output Framework::Preprocess::computeOutput(Dataset::Sample const & sample) const {
    static_assert(std::derived_from<Dataset::Sample, xai::nn::NNet::input_t>);
    static_assert(std::derived_from<Dataset::Output::Values, xai::nn::NNet::output_t>);

    auto & network = framework.getNetwork();

    Dataset::Output::Values outputValues = xai::nn::computeOutput(sample, network);
    auto label = computeClassificationLabel(outputValues);

    return {.classificationLabel = label, .values = std::move(outputValues)};
}

bool Framework::Preprocess::isBinaryClassification(Dataset::Output::Values const & values) {
    assert(not values.empty());
    assert(values.size() != 2);
    return (values.size() == 1);
}

Dataset::Classification::Label
Framework::Preprocess::computeClassificationLabel(Dataset::Output::Values const & values) {
    if (isBinaryClassification(values)) {
        return computeBinaryClassificationLabel(values);
    } else {
        return computeNonBinaryClassificationLabel(values);
    }
}

Dataset::Classification::Label
Framework::Preprocess::computeBinaryClassificationLabel(Dataset::Output::Values const & values) {
    assert(values.size() == 1);
    auto const val = values.front();
    return (val < 0) ? 0 : 1;
}

Dataset::Classification::Label
Framework::Preprocess::computeNonBinaryClassificationLabel(Dataset::Output::Values const & values) {
    auto const it = std::ranges::max_element(values);
    auto dist = std::distance(values.begin(), it);
    return dist;
}
} // namespace xspace
