#ifndef XSPACE_EXPLANATION_H
#define XSPACE_EXPLANATION_H

#include "PartialExplanation.h"

#include <xspace/common/Core.h>

namespace xspace {
class Explanation : public PartialExplanation {
public:
    using PartialExplanation::PartialExplanation;

    virtual void clear() {}

    void swap(Explanation &) {}

    std::size_t getFixedCount() const { return computeFixedCount(); }

    virtual Float getRelativeVolume() const = 0;
    virtual Float getRelativeVolumeSkipFixed() const = 0;

protected:
    virtual std::size_t computeFixedCount() const = 0;
};
} // namespace xspace

#endif // XSPACE_EXPLANATION_H
