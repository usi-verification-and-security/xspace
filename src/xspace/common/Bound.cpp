#include "Bound.h"

namespace xspace {
void Bound::print(std::ostream & os, PrintConfig const & conf) const {
    if (not conf.reverse) {
        printRegular(os);
    } else {
        printReverse(os);
    }
}

void Bound::printRegular(std::ostream & os) const {
    std::visit(
        [&](auto & t) {
            t.print(os);
            os << ' ' << value;
        },
        op);
}

void Bound::printReverse(std::ostream & os) const {
    std::visit(
        [&](auto & t) {
            os << value << ' ';
            t.printReverse(os);
        },
        op);
}

void Bound::Eq::print(std::ostream & os) const {
    os << "=";
}
void Bound::LtEq::print(std::ostream & os) const {
    os << "<=";
}
void Bound::LtEq::printReverse(std::ostream & os) const {
    os << ">=";
}

void Interval::print(std::ostream & os) const {
    os << '[' << lower;
    if (not isPoint()) { os << ',' << upper; }
    os << ']';
}

VarBound::VarBound(std::string_view var, LowerBound lo, UpperBound hi)
    : VarBound(lo.getValue() == hi.getValue() ? VarBound{var, std::move(lo), std::move(hi), ConsPointTag{}}
                                              : VarBound{var, std::move(lo), std::move(hi), ConsIntervalTag{}}) {}

VarBound::VarBound(std::string_view var, Bound bnd1, Bound bnd2)
    : VarBound(bnd1.isLower() ? VarBound{var, static_cast<LowerBound &&>(std::move(bnd1)),
                                         static_cast<UpperBound &&>(std::move(bnd2))}
                              : VarBound{var, static_cast<LowerBound &&>(std::move(bnd2)),
                                         static_cast<UpperBound &&>(std::move(bnd1))}) {}

void VarBound::print(std::ostream & os) const {
    if (not isInterval()) {
        printBound(os, firstBound);
        return;
    }

    // x >= l && x <= u -> l <= x <= u
    firstBound.print(os, {.reverse = true});
    os << ' ';
    printBound(os, *optSecondBound);
}

void VarBound::printBound(std::ostream & os, Bound const & bnd, Bound::PrintConfig const & conf) const {
    os << varName << ' ';
    bnd.print(os, conf);
}
} // namespace xspace
