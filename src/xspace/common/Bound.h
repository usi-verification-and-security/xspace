#ifndef XSPACE_BOUND_H
#define XSPACE_BOUND_H

#include "Core.h"

#include <cassert>
#include <iosfwd>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

namespace xspace {
class Bound {
public:
    enum class Type { eq, lteq, gteq };

    struct EqTag {};
    struct LtEqTag {};
    struct GtEqTag {};

    using LowerTag = GtEqTag;
    using UpperTag = LtEqTag;

    static constexpr Type lowerType = Type::gteq;
    static constexpr Type upperType = Type::lteq;

    // Run-time type
    explicit Bound(Type const & type, Float val)
        : Bound(type == Type::eq     ? Bound(EqTag{}, val)
                : type == Type::lteq ? Bound(LtEqTag{}, val)
                                     : Bound(GtEqTag{}, val)) {}

    // Compile-time type
    explicit Bound(EqTag t, Float val) : Bound(t, val, OpConsTag{}) {}
    explicit Bound(LtEqTag t, Float val) : Bound(t, val, OpConsTag{}) {}
    explicit Bound(GtEqTag t, Float val) : Bound(t, val, OpConsTag{}) {}

    bool isEq() const { return std::holds_alternative<Eq>(op); }
    bool isLtEq() const { return std::holds_alternative<LtEq>(op); }
    bool isGtEq() const { return std::holds_alternative<GtEq>(op); }

    bool isLower() const { return isGtEq(); }
    bool isUpper() const { return isLtEq(); }

    Float getValue() const { return value; }

    char const * getSymbol() const {
        return std::visit([](auto & t) { return t.getSymbol(); }, op);
    }
    char const * getReverseSymbol() const {
        return std::visit([](auto & t) { return t.getReverseSymbol(); }, op);
    }

    void print(std::ostream & os) const { printRegular(os); }
    void printReverse(std::ostream &) const;

protected:
    struct Eq {
        char const * getSymbol() const { return "="; }
        char const * getReverseSymbol() const { return getSymbol(); }
    };
    struct LtEq {
        char const * getSymbol() const { return "<="; }
        char const * getReverseSymbol() const { return ">="; }
    };
    struct GtEq : LtEq {
        char const * getSymbol() const { return LtEq::getReverseSymbol(); }
        char const * getReverseSymbol() const { return LtEq::getSymbol(); }
    };

    template<typename T>
    using OpTp =
        std::conditional_t<std::is_same_v<T, EqTag>, Eq, std::conditional_t<std::is_same_v<T, LtEqTag>, LtEq, GtEq>>;

    using Op = std::variant<Eq, LtEq, GtEq>;

    struct OpConsTag {};

    template<typename T>
    explicit Bound(T, Float val, OpConsTag) : op{OpTp<T>{}},
                                              value{val} {}

    void printRegular(std::ostream &) const;

    Op op;

    Float value;
};

struct EqBound : public Bound {
    explicit EqBound(Float val) : Bound(EqTag{}, val) {}
};
struct LtEqBound : public Bound {
    explicit LtEqBound(Float val) : Bound(LtEqTag{}, val) {}
};
struct GtEqBound : public Bound {
    explicit GtEqBound(Float val) : Bound(GtEqTag{}, val) {}
};

using LowerBound = GtEqBound;
using UpperBound = LtEqBound;

class Interval {
public:
    struct Range {
        Float lower;
        Float upper;
    };

    Interval(Range rng) : range{std::move(rng)} { assert(size() >= 0); }
    Interval(Float lo, Float hi) : Interval(Range{lo, hi}) {}
    Interval(Float val) : Interval(val, val) {}

    constexpr bool isPoint() const { return size() == 0; }

    constexpr Range const & getRange() const { return range; }

    constexpr Float getLower() const { return range.lower; }
    constexpr Float getUpper() const { return range.upper; }

    Float getValue() const { return getLower(); }

    constexpr Float size() const { return getUpper() - getLower(); }

    void print(std::ostream &) const;

protected:
    Range range;
};

class VarBound {
public:
    explicit VarBound(std::string_view var, Float val) : VarBound(var, EqBound{val}) {}
    explicit VarBound(std::string_view var, Interval ival)
        : VarBound(ival.isPoint() ? VarBound{var, ival.getValue()}
                                  : VarBound{var, LowerBound{ival.getLower()}, UpperBound{ival.getUpper()}}) {}
    explicit VarBound(std::string_view var, Bound bnd) : varName{var}, firstBound{std::move(bnd)} {}
    explicit VarBound(std::string_view var, LowerBound, UpperBound);
    explicit VarBound(std::string_view var, Bound, Bound);

    bool isInterval() const {
        assert(not isIntervalImpl() or firstBound.isLower());
        assert(not isIntervalImpl() or optSecondBound->isUpper());
        assert(not isIntervalImpl() or firstBound.getValue() != optSecondBound->getValue());
        return isIntervalImpl();
    }

    bool isPoint() const {
        assert(not isPointImpl() or not isInterval());
        return isPointImpl();
    }

    std::string_view getVarName() const { return varName; }

    Bound const & getBound() const { return firstBound; }

    LowerBound const & getLowerBound() const {
        auto & bnd = getBound();
        assert(bnd.isLower());
        return static_cast<LowerBound const &>(bnd);
    }

    UpperBound const & getUpperBound() const {
        assert(isInterval());
        auto & bnd = *optSecondBound;
        assert(bnd.isUpper());
        return static_cast<UpperBound const &>(bnd);
    }

    std::optional<Interval> tryGetInterval() const;

    void print(std::ostream & os) const { printRegular(os); }
    void printSmtLib2(std::ostream &) const;

protected:
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

void printSmtLib2AsRational(std::ostream &, Float);
} // namespace xspace

#endif // XSPACE_BOUND_H
