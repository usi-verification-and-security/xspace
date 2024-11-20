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
    enum class PrintFormat { bounds, smtlib2, intervals };

    struct PrintConfig {
        char delim = '\n';
        bool includeAll = false;
    };

    using AllVarBounds = std::vector<std::optional<VarBound>>;

    IntervalExplanation(Framework const &);

    auto begin() const { return varIdxToVarBoundMap.cbegin(); }
    auto end() const { return varIdxToVarBoundMap.cend(); }

    std::size_t size() const { return varIdxToVarBoundMap.size(); }

    AllVarBounds const & getAllVarBounds() const { return allVarBounds; }

    VarIdx getIdx(std::optional<VarBound> const & elem) const { return &elem - allVarBounds.data(); }

    std::optional<VarBound> const & tryGetVarBound(VarIdx idx) const {
        assert(idx < allVarBounds.size());
        return allVarBounds[idx];
    }

    void clear();

    void swap(IntervalExplanation &);

    void insertBound(VarIdx, Bound);

    bool eraseVarBound(VarIdx);

    std::size_t getFixedCount() const { return computeFixedCount(); }

    Float getRelativeVolume() const;
    Float getRelativeVolumeSkipFixed() const;

    void print(std::ostream &, PrintFormat const &, PrintConfig const &) const;
    void print(std::ostream &, PrintFormat const &) const;
    void print(std::ostream &, PrintConfig const &) const;
    void printSmtLib2(std::ostream &, PrintConfig const &) const;
    void printIntervals(std::ostream &, PrintConfig const &) const;
    void print(std::ostream & os) const { print(os, PrintConfig{}); }
    void printSmtLib2(std::ostream & os) const { printSmtLib2(os, defaultSmtLib2PrintConfig); }
    void printIntervals(std::ostream & os) const { printIntervals(os, defaultIntervalsPrintConfig); }

protected:
    using VarIdxToVarBoundMap = std::map<VarIdx, VarBound>;

    static constexpr PrintConfig defaultSmtLib2PrintConfig{.delim = ' ', .includeAll = false};
    static constexpr PrintConfig defaultIntervalsPrintConfig{.delim = ' ', .includeAll = true};

    Interval makeDomainInterval(VarIdx) const;

    Interval optVarBoundToInterval(VarIdx, std::optional<VarBound> const &) const;
    Interval varBoundToInterval(VarIdx, VarBound const &) const;

    std::size_t computeFixedCount() const;

    Framework const & framework;

    AllVarBounds allVarBounds;
    VarIdxToVarBoundMap varIdxToVarBoundMap{};

private:
    VarBound makeVarBound(VarIdx, auto &&...) const;

    template<bool skipFixed>
    Float computeRelativeVolumeTp() const;

    template<PrintFormat>
    void printTp(std::ostream &, PrintConfig const &) const;
    static void printElem(std::ostream &, PrintConfig const &, VarBound const &);
    static void printElemSmtLib2(std::ostream &, PrintConfig const &, VarBound const &);
    void printElemInterval(std::ostream &, PrintConfig const &, VarIdx, VarBound const &) const;
    void printElem(std::ostream &, PrintConfig const &, VarIdx, std::optional<VarBound> const &) const;
    void printElemSmtLib2(std::ostream &, PrintConfig const &, VarIdx, std::optional<VarBound> const &) const;
    void printElemInterval(std::ostream &, PrintConfig const &, VarIdx, std::optional<VarBound> const &) const;
};
} // namespace xspace

#endif // XSPACE_EXPLANATION_H
