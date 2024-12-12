#ifndef XSPACE_VARBOUND_H
#define XSPACE_VARBOUND_H

#include <xspace/common/Bound.h>
#include <xspace/common/Interval.h>
#include <xspace/common/Var.h>

#include <cassert>
#include <iosfwd>
#include <optional>
#include <type_traits>
#include <variant>

namespace xspace {
class Framework;

class VarBound {
public:
    explicit VarBound(Framework const & fw, VarIdx idx, Float val) : VarBound(fw, idx, EqBound{val}) {}
    explicit VarBound(Framework const & fw, VarIdx idx, Interval const & ival)
        : VarBound(fw, idx, LowerBound{ival.getLower()}, UpperBound{ival.getUpper()}) {}
    explicit VarBound(Framework const &, VarIdx, EqBound);
    explicit VarBound(Framework const &, VarIdx, LowerBound);
    explicit VarBound(Framework const &, VarIdx, UpperBound);
    explicit VarBound(Framework const &, VarIdx, Bound);
    explicit VarBound(Framework const &, VarIdx, LowerBound, UpperBound);
    explicit VarBound(Framework const &, VarIdx, Bound, Bound);

    bool isInterval() const {
        assert(not isIntervalImpl() or firstBound.isLower());
        assert(not isIntervalImpl() or optSecondBound->isUpper());
        assert(not isIntervalImpl() or firstBound.getValue() < optSecondBound->getValue());
        return isIntervalImpl();
    }

    bool isPoint() const {
        assert(not isPointImpl() or not isInterval());
        return isPointImpl();
    }

    VarIdx getVarIdx() const { return varIdx; }

    // If it holds just a single LowerBound or UpperBound, it must be accessed from here
    Bound const & getBound() const { return firstBound; }

    LowerBound const & getIntervalLower() const {
        assert(isInterval());
        auto & bnd = getBound();
        assert(bnd.isLower());
        return static_cast<LowerBound const &>(bnd);
    }

    UpperBound const & getIntervalUpper() const {
        assert(isInterval());
        auto & bnd = *optSecondBound;
        assert(bnd.isUpper());
        return static_cast<UpperBound const &>(bnd);
    }

    // Recommended but not necessary to prefer over getBound
    EqBound const & getPoint() const {
        assert(isPoint());
        return static_cast<EqBound const &>(getBound());
    }

    Float size() const { return toInterval().size(); }

    Interval getInterval() const;
    std::optional<Interval> tryGetIntervalOrPoint() const;

    Interval toInterval() const;

    void insertBound(Bound);
    void insertLowerBound(LowerBound);
    void insertUpperBound(UpperBound);

    void eraseLowerBound();
    void eraseUpperBound();

    void print(std::ostream & os) const { printRegular(os); }
    void printSmtLib2(std::ostream &) const;

protected:
    bool isValid() const;

    void printRegular(std::ostream &) const;
    void printRegularBound(std::ostream &, Bound const &) const;

    void printSmtLib2Bound(std::ostream &, Bound const &) const;

    Framework const * frameworkPtr;

    VarIdx varIdx;

    //+ it should be more efficient to either store `Interval` or a single `Bound`
    Bound firstBound;
    std::optional<Bound> optSecondBound{};

private:
    struct ConsOneBoundTag {};
    struct ConsPointTag {};
    struct ConsIntervalTag {};

    explicit VarBound(Framework const &, VarIdx, Bound &&, ConsOneBoundTag);
    explicit VarBound(Framework const &, VarIdx, LowerBound &&, UpperBound &&, ConsPointTag);
    explicit VarBound(Framework const &, VarIdx, LowerBound &&, UpperBound &&, ConsIntervalTag);

    bool isIntervalImpl() const { return optSecondBound.has_value(); }

    bool isPointImpl() const { return getBound().isEq(); }
};
} // namespace xspace

#endif // XSPACE_VARBOUND_H
