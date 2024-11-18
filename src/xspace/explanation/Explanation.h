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

    Float relativeVolume() const { return computeRelativeVolume(); }

    void print(std::ostream &) const;

protected:
    using VarIdxToVarBoundMap = std::map<VarIdx, VarBound>;

    VarBound makeVarBound(VarIdx, Bound) const;

    Interval varBoundToInterval(VarIdx, VarBound const &) const;

    Float computeRelativeVolume() const;

    Framework const & framework;

    AllVarBounds allVarBounds;
    VarIdxToVarBoundMap varIdxToVarBoundMap{};
};
} // namespace xspace

#endif // XSPACE_EXPLANATION_H
