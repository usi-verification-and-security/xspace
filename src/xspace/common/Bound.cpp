#include "Bound.h"

#include "Print.h"

//+ ideally get rid of these
#include <common/FastRational.h>
#include <common/StringConv.h>

#include <ostream>

namespace xspace {
void Bound::printRegular(std::ostream & os) const {
    std::visit([&](auto & t) { os << t.getSymbol() << ' ' << value; }, op);
}

void Bound::printReverse(std::ostream & os) const {
    std::visit([&](auto & t) { os << value << ' ' << t.getReverseSymbol(); }, op);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Interval::print(std::ostream & os) const {
    os << '[' << lower << ',' << upper << ']';
}

////////////////////////////////////////////////////////////////////////////////////////////////////

VarBound::VarBound(std::string_view var, LowerBound lo, UpperBound hi)
    : VarBound(lo.getValue() == hi.getValue() ? VarBound{var, std::move(lo), std::move(hi), ConsPointTag{}}
                                              : VarBound{var, std::move(lo), std::move(hi), ConsIntervalTag{}}) {}

VarBound::VarBound(std::string_view var, Bound bnd1, Bound bnd2)
    : VarBound(bnd1.isLower() ? VarBound{var, static_cast<LowerBound &&>(std::move(bnd1)),
                                         static_cast<UpperBound &&>(std::move(bnd2))}
                              : VarBound{var, static_cast<LowerBound &&>(std::move(bnd2)),
                                         static_cast<UpperBound &&>(std::move(bnd1))}) {}

std::optional<Interval> VarBound::tryGetInterval() const {
    if (isInterval()) { return Interval{firstBound.getValue(), optSecondBound->getValue()}; }
    if (isPoint()) { return Interval{firstBound.getValue()}; }
    return std::nullopt;
}

void VarBound::printRegular(std::ostream & os) const {
    if (not isInterval()) {
        printRegularBound(os, firstBound);
        return;
    }

    // x >= l && x <= u -> l <= x <= u
    firstBound.printReverse(os);
    os << ' ';
    printRegularBound(os, *optSecondBound);
}

void VarBound::printRegularBound(std::ostream & os, Bound const & bnd) const {
    os << varName << ' ' << bnd;
}

void VarBound::printSmtLib2(std::ostream & os) const {
    bool const ival = isInterval();

    if (ival) { os << "(and "; }
    printSmtLib2Bound(os, firstBound);
    if (ival) {
        printSmtLib2Bound(os, *optSecondBound);
        os << ')';
    }
}

void VarBound::printSmtLib2Bound(std::ostream & os, Bound const & bnd) const {
    os << '(' << bnd.getSymbol() << ' ' << varName << ' ';
    printSmtLib2AsRational(os, bnd.getValue());
    os << ')';
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
    //+ to be consistent with BasicVerix, but ideally should not depend on OpenSMT
    opensmt::FastRational floatToFastRational(Float value) {
        //! approximation
        auto s = std::to_string(value);
        char * rationalString;
        opensmt::stringToRational(rationalString, s.c_str());
        auto res = opensmt::FastRational(rationalString);
        free(rationalString);
        return res;
    }
} // namespace

void printSmtLib2AsRational(std::ostream & os, Float val) {
    auto const rat = floatToFastRational(val);
    os << rat;
}
} // namespace xspace
