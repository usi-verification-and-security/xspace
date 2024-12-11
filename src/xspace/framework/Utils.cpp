#include "Utils.h"

namespace xspace {
Interval varBoundToInterval(Framework const & framework, VarIdx idx, VarBound const & varBnd) {
    if (auto optIval = varBnd.tryGetIntervalOrPoint()) { return std::move(*optIval); }
    assert(not varBnd.isPoint());
    assert(not varBnd.isInterval());

    auto & bnd = varBnd.getBound();
    assert(bnd.isLower() xor bnd.isUpper());
    bool const isLower = bnd.isLower();

    auto & network = framework.getNetwork();
    Float val = bnd.getValue();
    auto ival =
        isLower ? Interval{val, network.getInputUpperBound(idx)} : Interval{network.getInputLowerBound(idx), val};
    assert(not ival.isPoint());

    return ival;
}
} // namespace xspace
