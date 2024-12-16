#ifndef XSPACE_DATASET_H
#define XSPACE_DATASET_H

#include <xspace/common/Core.h>
#include <xspace/common/Var.h>

#include <cassert>
#include <iosfwd>
#include <string_view>
#include <vector>

namespace xspace {
class Dataset {
public:
    struct Sample : std::vector<Float> {
        using Idx = size_type;

        using vector::vector;

        void print(std::ostream &) const;
    };

    using Samples = std::vector<Sample>;

    struct Classification {
        using Label = std::size_t;

        Label label;
    };

    using Classifications = std::vector<Classification>;

    struct Output {
        using Values = std::vector<Float>;

        Classification::Label classificationLabel;
        Values values{};
    };

    using Outputs = std::vector<Output>;

    Dataset(std::string_view fileName);

    std::size_t size() const { return getSamples().size(); }

    Samples const & getSamples() const { return samples; }
    Sample const & getSample(Sample::Idx idx) const {
        assert(idx < size());
        return samples[idx];
    }

    Classifications const & getExpectedClassifications() const { return expectedClassifications; }
    Classification const & getExpectedClassification(Sample::Idx idx) const {
        assert(idx < size());
        return expectedClassifications[idx];
    }

    void setComputedOutputs(Outputs);

    Outputs const & getComputedOutputs() const { return computedOutputs; }
    Output const & getComputedOutput(Sample::Idx idx) const {
        assert(idx < size());
        return computedOutputs[idx];
    }

    bool isCorrect(Sample::Idx);

protected:
    Samples samples{};

    Classifications expectedClassifications{};

    Outputs computedOutputs{};
};
} // namespace xspace

#endif // XSPACE_DATASET_H
