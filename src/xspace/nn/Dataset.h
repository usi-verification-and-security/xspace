#ifndef XSPACE_DATASET_H
#define XSPACE_DATASET_H

#include <xspace/common/Core.h>
#include <xspace/common/Var.h>

#include <iosfwd>
#include <string_view>
#include <vector>

namespace xspace {
class Dataset {
public:
    struct Sample;

    using Samples = std::vector<Sample>;
    using Outputs = std::vector<VarIdx>;

    Dataset(std::string_view fileName);

    std::size_t size() const { return getSamples().size(); }

    Samples const & getSamples() const { return samples; }
    Outputs const & getOutputs() const { return outputs; }

protected:
    Samples samples{};
    Outputs outputs{};
};

struct Dataset::Sample : std::vector<Float> {
    using vector::vector;

    void print(std::ostream &) const;
};
} // namespace xspace

#endif // XSPACE_DATASET_H
