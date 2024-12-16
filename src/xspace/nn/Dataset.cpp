#include "Dataset.h"

#include <filesystem>
#include <fstream>
#include <numeric>
#include <ranges>
#include <sstream>
#include <string>
#include <type_traits>

#ifndef NDEBUG
#include <cmath>
#endif

namespace xspace {
Dataset::Dataset(std::string_view fileName) {
    std::ifstream file{std::string{fileName}};
    if (not file.good()) { throw std::ifstream::failure{"Could not open dataset file "s + std::string{fileName}}; }

    // Read the first line to skip the header
    std::string header;
    getline(file, header);

    std::string line;
    Sample::Idx idx{};
    while (std::getline(file, line)) {
        std::istringstream ss{line};
        std::string field;
        Sample sample;
        while (std::getline(ss, field, ',')) {
            sample.push_back(std::stof(field));
        }
        assert(not sample.empty());
        Float expectedClassFloat = sample.back();
        assert(expectedClassFloat == std::floor(expectedClassFloat));
        sample.pop_back();
        samples.push_back(std::move(sample));

        Classification::Label label = expectedClassFloat;
        expectedClassifications.push_back({.label = label});
#ifndef NDEBUG
        classificationLabels.insert(label);
#endif

        SampleIndices & sampleIndicesOfClass = getSampleIndicesOfClass(label);
        sampleIndicesOfClass.push_back(idx);

        ++idx;
    }

    assert(not samples.empty());
    assert(size() == samples.size());
    assert(size() == expectedClassifications.size());

    assert(classificationSize() >= 2);
    assert(classificationSize() == classificationLabels.size());
    assert(classificationSize() == sampleIndicesOfClasses.size());
}

std::size_t Dataset::classificationSize() const {
    return sampleIndicesOfClasses.size();
}

Dataset::SampleIndices Dataset::getSampleIndices() const {
    SampleIndices indices(size());
    std::iota(indices.begin(), indices.end(), 0);
    return indices;
}

auto & Dataset::getSampleIndicesOfClassTp(auto & vec, Classification::Label label) {
    if constexpr (not std::is_const_v<std::remove_reference_t<decltype(vec)>>) {
        std::size_t const requiredSize = label + 1;
        if (vec.size() < requiredSize) { vec.resize(requiredSize); }
    }

    return vec[label];
}

Dataset::SampleIndices const & Dataset::getSampleIndicesOfClass(Classification::Label label) const {
    return getSampleIndicesOfClassTp(sampleIndicesOfClasses, label);
}

Dataset::SampleIndices & Dataset::getSampleIndicesOfClass(Classification::Label label) {
    return getSampleIndicesOfClassTp(sampleIndicesOfClasses, label);
}

Dataset::SampleIndices const & Dataset::getSampleIndicesOfExpectedClass(Classification::Label label) const {
    return getSampleIndicesOfClass(label);
}

void Dataset::setComputedOutputs(Outputs outs) {
    assert(outs.size() == size());
    computedOutputs = std::move(outs);

#ifndef NDEBUG
    for (auto & output : computedOutputs) {
        assert(classificationLabels.contains(output.classificationLabel));
        assert(not output.values.empty());
        if (output.values.size() == 1) {
            assert(classificationSize() == 2);
        } else {
            assert(output.values.size() == classificationSize());
        }
    }
#endif

    setCorrectAndIncorrectSamples();
}

bool Dataset::isCorrect(Sample::Idx idx) {
    auto const expectedLabel = getExpectedClassification(idx).label;
    auto const computedLabel = getComputedOutput(idx).classificationLabel;

    return expectedLabel == computedLabel;
}

void Dataset::setCorrectAndIncorrectSamples() {
    std::size_t size_ = size();
    for (Sample::Idx idx = 0; idx < size_; ++idx) {
        bool const correct = isCorrect(idx);

        auto & targetSampleIndices = correct ? correctSampleIndices : incorrectSampleIndices;
        targetSampleIndices.push_back(idx);

        auto const label = getExpectedClassification(idx).label;
        auto & targetSampleIndicesOfClass =
            correct ? getCorrectSampleIndicesOfClass(label) : getIncorrectSampleIndicesOfClass(label);
        targetSampleIndicesOfClass.push_back(idx);
    }

    assert(size() == correctSampleIndices.size() + incorrectSampleIndices.size());

    assert(classificationSize() == correctSampleIndicesOfClasses.size());
    assert(classificationSize() == incorrectSampleIndicesOfClasses.size());
}

Dataset::SampleIndices const & Dataset::getCorrectSampleIndices() const {
    return correctSampleIndices;
}

Dataset::SampleIndices const & Dataset::getIncorrectSampleIndices() const {
    return incorrectSampleIndices;
}

Dataset::SampleIndices const & Dataset::getCorrectSampleIndicesOfClass(Classification::Label label) const {
    return getSampleIndicesOfClassTp(correctSampleIndicesOfClasses, label);
}

Dataset::SampleIndices const & Dataset::getIncorrectSampleIndicesOfClass(Classification::Label label) const {
    return getSampleIndicesOfClassTp(incorrectSampleIndicesOfClasses, label);
}

Dataset::SampleIndices & Dataset::getCorrectSampleIndicesOfClass(Classification::Label label) {
    return getSampleIndicesOfClassTp(correctSampleIndicesOfClasses, label);
}

Dataset::SampleIndices & Dataset::getIncorrectSampleIndicesOfClass(Classification::Label label) {
    return getSampleIndicesOfClassTp(incorrectSampleIndicesOfClasses, label);
}

Dataset::SampleIndices const & Dataset::getCorrectSampleIndicesOfExpectedClass(Classification::Label label) const {
    return getCorrectSampleIndicesOfClass(label);
}

Dataset::SampleIndices const & Dataset::getIncorrectSampleIndicesOfExpectedClass(Classification::Label label) const {
    return getIncorrectSampleIndicesOfClass(label);
}

void Dataset::Sample::print(std::ostream & os) const {
    assert(not empty());
    os << front();
    for (Float val : *this | std::views::drop(1)) {
        os << ',' << val;
    }
}
} // namespace xspace
