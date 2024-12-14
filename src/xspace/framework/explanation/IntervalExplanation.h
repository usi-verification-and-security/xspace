#ifndef XSPACE_IVALEXPLANATION_H
#define XSPACE_IVALEXPLANATION_H

#include "Explanation.h"

#include "VarBound.h"

#include <xspace/common/Bound.h>
#include <xspace/common/Interval.h>

#include <cassert>
#include <map>
#include <optional>
#include <type_traits>
#include <vector>

namespace xspace {
class IntervalExplanation : public Explanation {
public:
    enum class PrintFormat { smtlib2, bounds, intervals };

    struct PrintConfig {
        char delim;
        bool includeAll;
    };

    using AllVarBounds = std::vector<std::optional<VarBound>>;

    static constexpr PrintConfig defaultSmtLib2PrintConfig{.delim = ' ', .includeAll = false};
    static constexpr PrintConfig defaultBoundsPrintConfig{.delim = '\n', .includeAll = false};
    static constexpr PrintConfig defaultIntervalsPrintConfig{.delim = ' ', .includeAll = true};

    IntervalExplanation(Framework const &);

    auto begin() const { return varIdxToVarBoundMap.cbegin(); }
    auto end() const { return varIdxToVarBoundMap.cend(); }

    std::size_t varSize() const override { return varIdxToVarBoundMap.size(); }

    bool contains(VarIdx idx) const override { return tryGetVarBound(idx).has_value(); }

    AllVarBounds const & getAllVarBounds() const { return allVarBounds; }

    VarIdx getIdx(std::optional<VarBound> const & elem) const { return &elem - allVarBounds.data(); }

    std::optional<VarBound> const & tryGetVarBound(VarIdx idx) const {
        assert(idx < allVarBounds.size());
        return allVarBounds[idx];
    }

    void clear() override;

    void swap(IntervalExplanation &);

    void insertVarBound(VarBound);
    void insertBound(VarIdx, Bound);

    bool eraseVarBound(VarIdx);

    void setVarBound(VarBound);
    void setVarBound(VarIdx, std::optional<VarBound>);

    Float getRelativeVolume() const override;
    Float getRelativeVolumeSkipFixed() const override;

    void print(std::ostream & os) const override;
    void printSmtLib2(std::ostream & os) const override { printSmtLib2(os, defaultSmtLib2PrintConfig); }
    void printBounds(std::ostream & os) const { printBounds(os, defaultBoundsPrintConfig); }
    void printIntervals(std::ostream & os) const { printIntervals(os, defaultIntervalsPrintConfig); }
    void print(std::ostream &, PrintConfig const &) const;
    void printSmtLib2(std::ostream &, PrintConfig const &) const;
    void printBounds(std::ostream &, PrintConfig const &) const;
    void printIntervals(std::ostream &, PrintConfig const &) const;

protected:
    using VarIdxToVarBoundMap = std::map<VarIdx, VarBound>;

    std::optional<VarBound> & _tryGetVarBound(VarIdx idx) {
        auto & optVarBnd = tryGetVarBound(idx);
        return const_cast<std::remove_cvref_t<decltype(optVarBnd)> &>(optVarBnd);
    }

    std::size_t computeFixedCount() const override;

    PrintFormat const & getPrintFormat() const;

    AllVarBounds allVarBounds;
    VarIdxToVarBoundMap varIdxToVarBoundMap{};

private:
    template<bool skipFixed>
    Float computeRelativeVolumeTp() const;

    template<PrintFormat>
    void printTp(std::ostream &, PrintConfig const &) const;
    static void printElemSmtLib2(std::ostream &, PrintConfig const &, VarBound const &);
    static void printElemBounds(std::ostream &, PrintConfig const &, VarBound const &);
    void printElemInterval(std::ostream &, PrintConfig const &, VarBound const &) const;
    void printElemSmtLib2(std::ostream &, PrintConfig const &, VarIdx, std::optional<VarBound> const &) const;
    void printElemBounds(std::ostream &, PrintConfig const &, VarIdx, std::optional<VarBound> const &) const;
    void printElemInterval(std::ostream &, PrintConfig const &, VarIdx, std::optional<VarBound> const &) const;
};
} // namespace xspace

#endif // XSPACE_IVALEXPLANATION_H
