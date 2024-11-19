#ifndef XSPACE_EXPLANATION_H
#define XSPACE_EXPLANATION_H

#include <xspace/common/Bound.h>
#include <xspace/common/Core.h>
#include <xspace/common/Var.h>

#include <cassert>
#include <map>
#include <optional>
#include <vector>

namespace xspace {
class Framework;

class IntervalExplanation {
public:
    using AllVarBounds = std::vector<std::optional<VarBound>>;

    IntervalExplanation(Framework const &);

    auto begin() const { return varIdxToVarBoundMap.cbegin(); }
    auto end() const { return varIdxToVarBoundMap.cend(); }

    std::size_t size() const { return varIdxToVarBoundMap.size(); }

    AllVarBounds const & getAllVarBounds() const { return allVarBounds; }

    std::size_t getIdx(std::optional<VarBound> const & elem) const {
        return &elem - allVarBounds.data();
    }

    std::optional<VarBound> const & tryGetVarBound(VarIdx idx) const {
        assert(idx < allVarBounds.size());
        return allVarBounds[idx];
    }

    void clear();

    void swap(IntervalExplanation &);

    void insertBound(VarIdx, Bound);

    bool eraseVarBound(VarIdx);

    std::size_t getFixedCount() const { return computeFixedCount(); }

    Float getRelativeVolume() const { return computeRelativeVolumeTp<false>(); }
    Float getRelativeVolumeSkipFixed() const { return computeRelativeVolumeTp<true>(); }

    void print(std::ostream &) const;
    void printSmtLib2(std::ostream &) const;

protected:
    using VarIdxToVarBoundMap = std::map<VarIdx, VarBound>;

    VarBound makeVarBound(VarIdx, Bound) const;

    Interval varBoundToInterval(VarIdx, VarBound const &) const;

    std::size_t computeFixedCount() const;

    template <bool skipFixed>
    Float computeRelativeVolumeTp() const;

    Framework const & framework;

    AllVarBounds allVarBounds;
    VarIdxToVarBoundMap varIdxToVarBoundMap{};
};

extern template Float IntervalExplanation::computeRelativeVolumeTp<false>() const;
extern template Float IntervalExplanation::computeRelativeVolumeTp<true>() const;
} // namespace xspace

#endif // XSPACE_EXPLANATION_H
