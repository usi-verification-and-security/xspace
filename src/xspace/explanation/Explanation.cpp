#include "Explanation.h"

#include <xspace/common/Print.h>
#include <xspace/framework/Framework.h>

#include <algorithm>

namespace xspace {
IntervalExplanation::IntervalExplanation(Framework const & fw) : framework{fw}, allVarBounds(fw.varSize()) {}

VarBound IntervalExplanation::makeVarBound(VarIdx idx, Bound bnd) const {
    auto & name = framework.varName(idx);
    return VarBound{name, std::move(bnd)};
}

void IntervalExplanation::clear() {
    for (auto & optVarBnd : allVarBounds) {
        optVarBnd.reset();
    }
    varIdxToVarBoundMap.clear();
}

void IntervalExplanation::swap(IntervalExplanation & rhs) {
    allVarBounds.swap(rhs.allVarBounds);
    varIdxToVarBoundMap.swap(rhs.varIdxToVarBoundMap);
}

void IntervalExplanation::insertBound(VarIdx idx, Bound bnd) {
    assert(not allVarBounds.empty());
    assert(allVarBounds.size() == framework.varSize());
    assert(size() <= framework.varSize());

#ifndef NDEBUG
    auto & network = framework.getNetwork();
    Float domainLower = network.getInputLowerBound(idx);
    Float domainUpper = network.getInputUpperBound(idx);
    Float val = bnd.getValue();
    bool const valIsLower = (val == domainLower);
    bool const valIsUpper = (val == domainUpper);
    assert(not valIsLower or not valIsUpper);
    // It is worthless to assert bounds that already correspond to the bounds of the domain
    // assert(not valIsLower or not bnd.isLower());
    // assert(not valIsUpper or not bnd.isUpper());
    assert(not valIsLower or bnd.isEq());
    assert(not valIsUpper or bnd.isEq());
#endif

    auto & optVarBound = allVarBounds[idx];
    bool const contains = optVarBound.has_value();

    if (not contains) {
        VarBound newVarBound = makeVarBound(idx, std::move(bnd));
        optVarBound.emplace(newVarBound);
        [[maybe_unused]] auto [it, inserted] = varIdxToVarBoundMap.emplace(idx, std::move(newVarBound));
        assert(it != varIdxToVarBoundMap.end());
        assert(inserted);
        assert(size() <= framework.varSize());
        return;
    }

    VarBound & varBnd = *optVarBound;
    assert(varBnd.getVarName() == framework.varName(idx));
    assert(not varBnd.isInterval());
    assert(not varBnd.isPoint());
    Bound const & oldBnd = varBnd.getBound();

    assert(not bnd.isEq());
    assert(not oldBnd.isEq());
    assert(bnd.isLower() xor oldBnd.isLower());
    assert(bnd.isUpper() xor oldBnd.isUpper());
    varBnd = VarBound{varBnd.getVarName(), oldBnd, std::move(bnd)};

    auto it = varIdxToVarBoundMap.find(idx);
    assert(it != varIdxToVarBoundMap.end());
    VarBound & mapVarBnd = it->second;
    assert(mapVarBnd.getVarName() == varBnd.getVarName());
    mapVarBnd = varBnd;
}

bool IntervalExplanation::eraseVarBound(VarIdx idx) {
    auto & optVarBound = allVarBounds[idx];
    bool const contained = optVarBound.has_value();
    optVarBound.reset();
    [[maybe_unused]] std::size_t cnt = varIdxToVarBoundMap.erase(idx);
    assert(cnt == 0 or cnt == 1);
    assert(contained == (cnt == 1));

    return contained;
}

Interval IntervalExplanation::varBoundToInterval(VarIdx idx, VarBound const & varBnd) const {
    if (varBnd.isPoint()) { return varBnd.getBound().getValue(); }
    if (varBnd.isInterval()) { return {varBnd.getLowerBound().getValue(), varBnd.getUpperBound().getValue()}; }

    auto & bnd = varBnd.getBound();
    assert(bnd.isLower() xor bnd.isUpper());
    bool const isLower = bnd.isLower();

    auto & network = framework.getNetwork();
    Float const val = bnd.getValue();
    auto ival =
        isLower ? Interval{val, network.getInputUpperBound(idx)} : Interval{network.getInputLowerBound(idx), val};
    assert(ival.size() > 0);

    return ival;
}

std::size_t IntervalExplanation::computeFixedCount() const {
    return std::ranges::count_if(*this, [](auto const & pair) {
        auto & varBnd = pair.second;
        return varBnd.isPoint();
    });
}

template<bool skipFixed>
Float IntervalExplanation::computeRelativeVolumeTp() const {
    auto & network = framework.getNetwork();

    Float relVolume = 1;
    for (auto & [idx, varBnd] : varIdxToVarBoundMap) {
        Interval ival = varBoundToInterval(idx, varBnd);
        Float const size = ival.size();
        assert(size >= 0);
        if (size == 0) {
            if constexpr (skipFixed) {
                continue;
            } else {
                return 0;
            }
        }

        Float const domainSize = network.getInputUpperBound(idx) - network.getInputLowerBound(idx);
        assert(domainSize > 0);
        assert(size < domainSize);

        relVolume *= size / domainSize;
    }

    assert(relVolume > 0);
    assert(relVolume <= 1);
    return relVolume;
}

template Float IntervalExplanation::computeRelativeVolumeTp<false>() const;
template Float IntervalExplanation::computeRelativeVolumeTp<true>() const;

void IntervalExplanation::print(std::ostream & os) const {
    for (auto & [idx, varBnd] : varIdxToVarBoundMap) {
        os << varBnd << '\n';
    }
}

void IntervalExplanation::printSmtLib2(std::ostream & os) const {
    os << "(and";
    for (auto & [idx, varBnd] : varIdxToVarBoundMap) {
        os << ' ' << smtLib2Format(varBnd);
    }
    os << ')';
}
} // namespace xspace
