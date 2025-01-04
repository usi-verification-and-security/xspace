#ifndef XSPACE_IVALEXPLANATION_H
#define XSPACE_IVALEXPLANATION_H

#include "ConjunctExplanation.h"

#include "VarBound.h"

#include <xspace/common/Bound.h>
#include <xspace/common/Interval.h>

#include <cassert>

namespace xspace {
class IntervalExplanation : public ConjunctExplanation {
public:
    enum class PrintFormat { smtlib2, bounds, intervals };

    struct PrintConfig : ConjunctExplanation::PrintConfig {
        bool includeAll;
    };

    static constexpr PrintConfig defaultBoundsPrintConfig{{.delim = '\n'}, false};
    static constexpr PrintConfig defaultIntervalsPrintConfig{{.delim = ' '}, true};

    explicit IntervalExplanation(Framework const &);

    std::size_t size() const {
        assert(ConjunctExplanation::size() == frameworkPtr->varSize());
        return ConjunctExplanation::size();
    }

    VarBound const * tryGetVarBound(VarIdx idx) const {
        auto * expPtr = tryGetExplanation(idx);
        return castToVarBound(expPtr);
    }

    std::size_t varSize() const override;

    bool contains(VarIdx idx) const override { return bool(tryGetVarBound(idx)); }

    void insertVarBound(VarBound);
    void insertBound(VarIdx, Bound);

    void setVarBound(VarBound);
    void setVarBound(VarIdx idx, std::unique_ptr<VarBound> varBndPtr) { setExplanation(idx, std::move(varBndPtr)); }

    bool eraseVarBound(VarIdx idx) { return eraseExplanation(idx); }

    Float getRelativeVolume() const override;
    Float getRelativeVolumeSkipFixed() const override;

    void print(std::ostream & os) const override;
    void printBounds(std::ostream & os) const { printBounds(os, defaultBoundsPrintConfig); }
    void printIntervals(std::ostream & os) const { printIntervals(os, defaultIntervalsPrintConfig); }
    void print(std::ostream &, PrintConfig const &) const;
    void printBounds(std::ostream &, PrintConfig const &) const;
    void printIntervals(std::ostream &, PrintConfig const &) const;

protected:
    void insertExplanation(std::unique_ptr<PartialExplanation>) override;
    void setExplanation(std::size_t idx, std::unique_ptr<PartialExplanation>) override;
    using ConjunctExplanation::eraseExplanation;

    std::size_t computeFixedCount() const override;

    PrintFormat const & getPrintFormat() const;

private:
    static VarBound const * castToVarBound(PartialExplanation const * expPtr) {
        assert(not expPtr or dynamic_cast<VarBound const *>(expPtr));
        return static_cast<VarBound const *>(expPtr);
    }

    static VarBound * castToVarBound(PartialExplanation * expPtr) {
        assert(not expPtr or dynamic_cast<VarBound *>(expPtr));
        return static_cast<VarBound *>(expPtr);
    }

    VarBound * _tryGetVarBound(VarIdx idx) {
        auto * expPtr = getExplanationPtr(idx).get();
        return castToVarBound(expPtr);
    }

    template<bool skipFixed>
    Float computeRelativeVolumeTp() const;

    template<PrintFormat>
    void printTp(std::ostream &, PrintConfig const &) const;
    static void printElemBounds(std::ostream &, PrintConfig const &, VarBound const &);
    void printElemInterval(std::ostream &, PrintConfig const &, VarBound const &) const;
    void printElemBounds(std::ostream &, PrintConfig const &, VarIdx, VarBound const *) const;
    void printElemInterval(std::ostream &, PrintConfig const &, VarIdx, VarBound const *) const;
};
} // namespace xspace

#endif // XSPACE_IVALEXPLANATION_H
