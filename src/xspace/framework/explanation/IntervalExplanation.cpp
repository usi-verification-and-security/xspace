#include "IntervalExplanation.h"

#include "../Config.h"
#include "../Framework.h"

#include <xspace/common/Print.h>

#include <algorithm>

namespace xspace {
IntervalExplanation::IntervalExplanation(Framework const & fw) : Explanation{fw}, allVarBounds(fw.varSize()) {}

void IntervalExplanation::clear() {
    Explanation::clear();

    for (auto & optVarBnd : allVarBounds) {
        optVarBnd.reset();
    }
    varIdxToVarBoundMap.clear();
}

void IntervalExplanation::swap(IntervalExplanation & rhs) {
    Explanation::swap(rhs);

    allVarBounds.swap(rhs.allVarBounds);
    varIdxToVarBoundMap.swap(rhs.varIdxToVarBoundMap);
}

void IntervalExplanation::insertVarBound(VarBound varBnd) {
    VarIdx const idx = varBnd.getVarIdx();
    assert(not contains(idx));

    allVarBounds[idx].emplace(varBnd);
    [[maybe_unused]] auto [it, inserted] = varIdxToVarBoundMap.emplace(idx, std::move(varBnd));
    assert(it != varIdxToVarBoundMap.end());
    assert(inserted);
    assert(varSize() <= frameworkPtr->varSize());
}

void IntervalExplanation::insertBound(VarIdx idx, Bound bnd) {
    assert(not allVarBounds.empty());
    assert(allVarBounds.size() == frameworkPtr->varSize());
    assert(varSize() <= frameworkPtr->varSize());

#ifndef NDEBUG
    auto & network = frameworkPtr->getNetwork();
    Float domainLower = network.getInputLowerBound(idx);
    Float domainUpper = network.getInputUpperBound(idx);
    Float val = bnd.getValue();
    bool const valIsLower = (val == domainLower);
    bool const valIsUpper = (val == domainUpper);
    assert(not valIsLower or not valIsUpper);
    // It is worthless to assert bounds that already correspond to the bounds of the domain
    assert(not valIsLower or bnd.isEq());
    assert(not valIsUpper or bnd.isEq());
#endif

    auto & optVarBnd = _tryGetVarBound(idx);
    if (not optVarBnd.has_value()) {
        insertVarBound(VarBound{*frameworkPtr, idx, std::move(bnd)});
        return;
    }

    VarBound & varBnd = *optVarBnd;
    assert(not varBnd.isInterval());
    assert(not varBnd.isPoint());
    assert(not bnd.isEq());
    assert(bnd.isLower() xor varBnd.getBound().isLower());
    assert(bnd.isUpper() xor varBnd.getBound().isUpper());
    varBnd.insertBound(std::move(bnd));

    auto it = varIdxToVarBoundMap.find(idx);
    assert(it != varIdxToVarBoundMap.end());
    VarBound & mapVarBnd = it->second;
    assert(mapVarBnd.getVarIdx() == varBnd.getVarIdx());
    mapVarBnd = varBnd;
}

bool IntervalExplanation::eraseVarBound(VarIdx idx) {
    auto & optVarBnd = _tryGetVarBound(idx);
    bool const contained = optVarBnd.has_value();
    optVarBnd.reset();
    [[maybe_unused]] std::size_t cnt = varIdxToVarBoundMap.erase(idx);
    assert(cnt == 0 or cnt == 1);
    assert(contained == (cnt == 1));

    return contained;
}

void IntervalExplanation::setVarBound(VarBound varBnd) {
    VarIdx const idx = varBnd.getVarIdx();
    assert(contains(idx));
    auto & optVarBnd = _tryGetVarBound(idx);
    assert(optVarBnd.has_value());
    *optVarBnd = varBnd;

    auto it = varIdxToVarBoundMap.find(idx);
    assert(it != varIdxToVarBoundMap.end());
    it->second = std::move(varBnd);
}

void IntervalExplanation::setVarBound(VarIdx idx, std::optional<VarBound> optVarBnd) {
    if (optVarBnd.has_value()) {
        setVarBound(*std::move(optVarBnd));
        return;
    }

    assert(_tryGetVarBound(idx).has_value());
    _tryGetVarBound(idx).reset();
    [[maybe_unused]] std::size_t cnt = varIdxToVarBoundMap.erase(idx);
    assert(cnt == 1);
}

std::size_t IntervalExplanation::computeFixedCount() const {
    return std::ranges::count_if(*this, [](auto const & pair) {
        auto & varBnd = pair.second;
        return varBnd.isPoint();
    });
}

Float IntervalExplanation::getRelativeVolume() const {
    return computeRelativeVolumeTp<false>();
}
Float IntervalExplanation::getRelativeVolumeSkipFixed() const {
    return computeRelativeVolumeTp<true>();
}

template<bool skipFixed>
Float IntervalExplanation::computeRelativeVolumeTp() const {
    Float relVolume = 1;
    for (auto & [idx, varBnd] : varIdxToVarBoundMap) {
        Float const size = varBnd.size();
        assert(size >= 0);
        if (size == 0) {
            if constexpr (skipFixed) {
                continue;
            } else {
                return 0;
            }
        }

        Float const domainSize = frameworkPtr->getDomainInterval(idx).size();
        assert(domainSize > 0);
        assert(size < domainSize);

        relVolume *= size / domainSize;
    }

    assert(relVolume > 0);
    assert(relVolume <= 1);
    return relVolume;
}

IntervalExplanation::PrintFormat const & IntervalExplanation::getPrintFormat() const {
    return frameworkPtr->getConfig().getPrintingIntervalExplanationsFormat();
}

void IntervalExplanation::print(std::ostream & os) const {
    PrintFormat const & type = getPrintFormat();
    using enum PrintFormat;
    switch (type) {
        case smtlib2:
            printSmtLib2(os, defaultSmtLib2PrintConfig);
            return;
        case bounds:
            printBounds(os, defaultBoundsPrintConfig);
            return;
        case intervals:
            printIntervals(os, defaultIntervalsPrintConfig);
            return;
    }
}

void IntervalExplanation::print(std::ostream & os, PrintConfig const & conf) const {
    PrintFormat const & type = getPrintFormat();
    using enum PrintFormat;
    switch (type) {
        case smtlib2:
            printSmtLib2(os, conf);
            return;
        case bounds:
            printBounds(os, conf);
            return;
        case intervals:
            printIntervals(os, conf);
            return;
    }
}

void IntervalExplanation::printSmtLib2(std::ostream & os, PrintConfig const & conf) const {
    printTp<PrintFormat::smtlib2>(os, conf);
}

void IntervalExplanation::printBounds(std::ostream & os, PrintConfig const & conf) const {
    printTp<PrintFormat::bounds>(os, conf);
}

void IntervalExplanation::printIntervals(std::ostream & os, PrintConfig const & conf) const {
    printTp<PrintFormat::intervals>(os, conf);
}

template<IntervalExplanation::PrintFormat type>
void IntervalExplanation::printTp(std::ostream & os, PrintConfig const & conf) const {
    constexpr bool isSmtLib2 = (type == PrintFormat::smtlib2);
    constexpr bool isBounds = (type == PrintFormat::bounds);
    constexpr bool isIntervals = (type == PrintFormat::intervals);

    if constexpr (isSmtLib2) { os << "(and"; }

    if (not conf.includeAll) {
        for (auto & [idx, varBnd] : varIdxToVarBoundMap) {
            if constexpr (isSmtLib2) {
                printElemSmtLib2(os, conf, varBnd);
            } else if constexpr (isBounds) {
                printElemBounds(os, conf, varBnd);
            } else {
                static_assert(isIntervals);
                printElemInterval(os, conf, idx, varBnd);
            }
        }
    } else {
        for (auto & optVarBnd : allVarBounds) {
            [[maybe_unused]] VarIdx const idx = getIdx(optVarBnd);
            if constexpr (isSmtLib2) {
                assert(false);
            } else if constexpr (isBounds) {
                printElemBounds(os, conf, idx, optVarBnd);
            } else {
                static_assert(isIntervals);
                printElemInterval(os, conf, idx, optVarBnd);
            }
        }
    }

    if constexpr (isSmtLib2) { os << ')'; }
}

void IntervalExplanation::printElemSmtLib2(std::ostream & os, PrintConfig const & conf, VarBound const & varBnd) {
    os << conf.delim << smtLib2Format(varBnd);
}

void IntervalExplanation::printElemBounds(std::ostream & os, PrintConfig const & conf, VarBound const & varBnd) {
    os << varBnd << conf.delim;
}

void IntervalExplanation::printElemInterval(std::ostream & os, PrintConfig const & conf,
                                            VarBound const & varBnd) const {
    os << varBnd.toInterval() << conf.delim;
}

void IntervalExplanation::printElemBounds(std::ostream & os, PrintConfig const & conf, VarIdx idx,
                                          std::optional<VarBound> const & optVarBnd) const {
    if (optVarBnd.has_value()) {
        printElemBounds(os, conf, *optVarBnd);
    } else {
        os << frameworkPtr->getVarName(idx) << " free" << conf.delim;
    }
}

void IntervalExplanation::printElemInterval(std::ostream & os, PrintConfig const & conf, VarIdx idx,
                                            std::optional<VarBound> const & optVarBnd) const {
    if (optVarBnd.has_value()) {
        printElemInterval(os, conf, *optVarBnd);
    } else {
        os << frameworkPtr->getDomainInterval(idx) << conf.delim;
    }
}
} // namespace xspace
