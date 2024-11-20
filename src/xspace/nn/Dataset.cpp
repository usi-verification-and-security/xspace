#include "Dataset.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <sstream>
#include <string>

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
        Float output = sample.back();
        assert(output == std::floor(output));
        sample.resize(sample.size() - 1);
        samples.push_back(std::move(sample));
        outputs.push_back(output);
    }
    assert(not samples.empty());
}

void Dataset::Sample::print(std::ostream & os) const {
    assert(not empty());
    os << front();
    for (Float val : *this | std::views::drop(1)) {
        os << ',' << val;
    }
}
} // namespace xspace
