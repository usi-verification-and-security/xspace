#include "Dataset.h"

#include <filesystem>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>

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

        Classification::Label label = expectedClassFloat;
        expectedClassifications.push_back({.label = label});

        samples.push_back(std::move(sample));
    }

    assert(not samples.empty());
    assert(size() == samples.size());
    assert(size() == expectedClassifications.size());
}

void Dataset::setComputedOutputs(Outputs outs) {
    assert(outs.size() == size());
    computedOutputs = std::move(outs);
}

bool Dataset::isCorrect(Sample::Idx idx) {
    auto const expectedLabel = getExpectedClassification(idx).label;
    auto const computedLabel = getComputedOutput(idx).classificationLabel;

    return expectedLabel == computedLabel;
}

void Dataset::Sample::print(std::ostream & os) const {
    assert(not empty());
    os << front();
    for (Float val : *this | std::views::drop(1)) {
        os << ',' << val;
    }
}
} // namespace xspace
