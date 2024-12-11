#ifndef XSPACE_VARBOUND_H
#define XSPACE_VARBOUND_H

#include <xspace/common/Bound.h>
#include <xspace/common/Interval.h>

#include <cassert>
#include <iosfwd>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

namespace xspace {
class VarBound {
public:
    explicit VarBound(std::string_view var, Float val) : VarBound(var, EqBound{val}) {}
    explicit VarBound(std::string_view var, Interval ival)
        : VarBound(ival.isPoint() ? VarBound{var, ival.getValue()}
                                  : VarBound{var, LowerBound{ival.getLower()}, UpperBound{ival.getUpper()}}) {}
    explicit VarBound(std::string_view var, Bound bnd) : varName{var}, firstBound{std::move(bnd)} { assert(isValid()); }
    explicit VarBound(std::string_view var, LowerBound, UpperBound);
    explicit VarBound(std::string_view var, Bound, Bound);

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

    std::string_view getVarName() const { return varName; }

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

    Interval getInterval() const;
    std::optional<Interval> tryGetIntervalOrPoint() const;

    void print(std::ostream & os) const { printRegular(os); }
    void printSmtLib2(std::ostream &) const;

protected:
    // These include additional assertions
    bool isValid() const { return isInterval() or isPoint() or not isInterval(); }

    void printRegular(std::ostream &) const;
    void printRegularBound(std::ostream &, Bound const &) const;

    void printSmtLib2Bound(std::ostream &, Bound const &) const;

    std::string_view varName;

    //+ it should be more efficient to either store `Interval` or a single `Bound`
    Bound firstBound;
    std::optional<Bound> optSecondBound{};

private:
    struct ConsIntervalTag {};
    struct ConsPointTag {};

    explicit VarBound(std::string_view var, LowerBound && lo, [[maybe_unused]] UpperBound && hi, ConsPointTag)
        : VarBound(var, lo.getValue()) {
        assert(lo.isLower());
        assert(hi.isUpper());
        assert(lo.getValue() == hi.getValue());
    }
    explicit VarBound(std::string_view var, LowerBound && lo, UpperBound && hi, ConsIntervalTag)
        : varName{var},
          firstBound{std::move(lo)},
          optSecondBound{std::move(hi)} {
        assert(firstBound.isLower());
        assert(optSecondBound->isUpper());
        assert(firstBound.getValue() < optSecondBound->getValue());
    }

    bool isIntervalImpl() const { return optSecondBound.has_value(); }

    bool isPointImpl() const { return getBound().isEq(); }
};
} // namespace xspace

#endif // XSPACE_VARBOUND_H
