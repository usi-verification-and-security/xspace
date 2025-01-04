#include "PartialExplanation.h"

#include <utility>

namespace xspace {
std::size_t PartialExplanation::varSize() const {
    std::size_t const varSize_ = frameworkPtr->varSize();
    std::size_t cnt{};
    for (VarIdx idx = 0; idx < varSize_; ++idx) {
        if (contains(idx)) { ++cnt; }
    }
    return cnt;
}

void PartialExplanation::swap(PartialExplanation & rhs) {
    std::swap(frameworkPtr, rhs.frameworkPtr);
}
} // namespace xspace
