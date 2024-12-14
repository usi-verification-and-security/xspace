#include "Strategy.h"

#include <xspace/framework/Utils.h>
#include <xspace/framework/explanation/Explanation.h>
#include <xspace/framework/explanation/VarBound.h>

#include <verifiers/UnsatCoreVerifier.h>
#include <verifiers/Verifier.h>

#include <cassert>
#include <numeric>

namespace xspace {
Framework::Expand::Strategy::Strategy(Expand & exp, VarOrdering order) : expand{exp}, varOrdering{std::move(order)} {
    assert((varOrdering.type == VarOrdering::Type::manual) xor varOrdering.manualOrder.empty());
}

void Framework::Expand::Strategy::execute(Explanation & explanation) {
    executeInit(explanation);
    executeBody(explanation);
    executeFinish(explanation);
}

void Framework::Expand::Strategy::executeInit(Explanation &) {
    auto const & orderType = varOrdering.type;
    auto & varOrder = varOrdering.manualOrder;
    std::size_t const varSize = expand.framework.varSize();
    assert(varSize > 0);
    if (orderType == VarOrdering::Type::manual) {
        assert(varOrder.size() == varSize);
        return;
    }

    varOrder.resize(varSize);
    if (orderType == VarOrdering::Type::regular) {
        std::iota(varOrder.begin(), varOrder.end(), 0);
    } else {
        assert(orderType == VarOrdering::Type::reverse);
        std::iota(varOrder.rbegin(), varOrder.rend(), 0);
    }
}

void Framework::Expand::Strategy::assertVarBound(VarBound const & varBnd, bool splitEq) {
    VarIdx const idx = varBnd.getVarIdx();
    if (varBnd.isInterval()) {
        assertInnerInterval(idx, varBnd.getIntervalLower(), varBnd.getIntervalUpper());
        return;
    }

    if (varBnd.isPoint()) {
        assertPoint(idx, varBnd.getPoint(), splitEq);
        return;
    }

    auto & bnd = varBnd.getBound();
    assert(not bnd.isEq());
    assertBound(idx, bnd);
}

void Framework::Expand::Strategy::assertInterval(VarIdx idx, Interval const & ival) {
    if (ival.isPoint()) {
        assertPoint(idx, ival.getValue());
        return;
    }

    auto const [lo, hi] = ival.getBounds();
    auto & network = expand.framework.getNetwork();
    assert(lo >= network.getInputLowerBound(idx));
    assert(hi <= network.getInputUpperBound(idx));
    bool const isLower = (lo == network.getInputLowerBound(idx));
    bool const isUpper = (hi == network.getInputUpperBound(idx));
    if (isLower and isUpper) { return; }

    if (not isLower and not isUpper) {
        assertInnerInterval(idx, ival);
        return;
    }

    assert(isLower xor isUpper);
    if (isLower) {
        assertUpperBound(idx, UpperBound{hi});
    } else {
        assertLowerBound(idx, LowerBound{lo});
    }
}

void Framework::Expand::Strategy::assertInnerInterval(VarIdx idx, Interval const & ival) {
    LowerBound lo{ival.getLower()};
    UpperBound hi{ival.getUpper()};
    assertInnerInterval(idx, lo, hi);
}

void Framework::Expand::Strategy::assertInnerInterval(VarIdx idx, LowerBound const & lo, UpperBound const & hi) {
    assert(lo.getValue() < hi.getValue());

    assertLowerBound(idx, lo);
    assertUpperBound(idx, hi);
}

void Framework::Expand::Strategy::assertPoint(VarIdx idx, Float val) {
    assertPointNoSplit(idx, EqBound{val});
}

void Framework::Expand::Strategy::assertPoint(VarIdx idx, EqBound const & eq, bool splitEq) {
    if (not splitEq) {
        assertPointNoSplit(idx, eq);
    } else {
        assertPointSplit(idx, eq);
    }
}

void Framework::Expand::Strategy::assertPointNoSplit(VarIdx idx, EqBound const & eq) {
    assertEquality(idx, eq);
}

void Framework::Expand::Strategy::assertPointSplit(VarIdx idx, EqBound const & eq) {
    Float const val = eq.getValue();
    auto & network = expand.framework.getNetwork();
    bool const isLower = (val == network.getInputLowerBound(idx));
    bool const isUpper = (val == network.getInputUpperBound(idx));
    assert(not isLower or not isUpper);
    if (isLower or isUpper) {
        assertEquality(idx, eq);
        return;
    }

    assertLowerBound(idx, LowerBound{val});
    assertUpperBound(idx, UpperBound{val});
}

void Framework::Expand::Strategy::assertBound(VarIdx idx, Bound const & bnd) {
    if (bnd.isEq()) {
        assertEquality(idx, static_cast<EqBound const &>(bnd));
    } else if (bnd.isLower()) {
        assertLowerBound(idx, static_cast<LowerBound const &>(bnd));
    } else {
        assert(bnd.isUpper());
        assertUpperBound(idx, static_cast<UpperBound const &>(bnd));
    }
}

void Framework::Expand::Strategy::assertEquality(VarIdx idx, EqBound const & eq) {
    Float const val = eq.getValue();
    assert(val >= expand.framework.getNetwork().getInputLowerBound(idx));
    assert(val <= expand.framework.getNetwork().getInputUpperBound(idx));
    expand.verifierPtr->addEquality(0, idx, val, storeNamedTerms());
}

void Framework::Expand::Strategy::assertLowerBound(VarIdx idx, LowerBound const & lo) {
    Float const val = lo.getValue();
    assert(val > expand.framework.getNetwork().getInputLowerBound(idx));
    assert(val < expand.framework.getNetwork().getInputUpperBound(idx));
    expand.verifierPtr->addLowerBound(0, idx, val, storeNamedTerms());
}

void Framework::Expand::Strategy::assertUpperBound(VarIdx idx, UpperBound const & hi) {
    Float const val = hi.getValue();
    assert(val > expand.framework.getNetwork().getInputLowerBound(idx));
    assert(val < expand.framework.getNetwork().getInputUpperBound(idx));
    expand.verifierPtr->addUpperBound(0, idx, val, storeNamedTerms());
}

bool Framework::Expand::Strategy::checkFormsExplanation() {
    auto answer = expand.verifierPtr->check();
    assert(answer == xai::verifiers::Verifier::Answer::SAT or answer == xai::verifiers::Verifier::Answer::UNSAT);
    return (answer == xai::verifiers::Verifier::Answer::UNSAT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Framework::Expand::AbductiveStrategy::executeBody(Explanation & explanation) {
    auto & verifier = *expand.verifierPtr;

    assert(dynamic_cast<IntervalExplanation *>(&explanation));
    auto & iexplanation = static_cast<IntervalExplanation &>(explanation);

    std::size_t const varSize = expand.framework.varSize();
    for (VarIdx idxToOmit : varOrdering.manualOrder) {
        verifier.push();
        for (VarIdx idx = 0; idx < varSize; ++idx) {
            if (idx == idxToOmit) { continue; }
            auto & optVarBnd = iexplanation.tryGetVarBound(idx);
            if (not optVarBnd.has_value()) { continue; }

            auto & varBnd = *optVarBnd;
            assertVarBound(varBnd);
        }
        bool const ok = checkFormsExplanation();
        verifier.pop();
        // It is no longer explanation after the removal -> we cannot remove it
        if (not ok) { continue; }
        iexplanation.eraseVarBound(idxToOmit);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Framework::Expand::TrialAndErrorStrategy::executeBody(Explanation & explanation) {
    auto & verifier = *expand.verifierPtr;

    assert(dynamic_cast<IntervalExplanation *>(&explanation));
    auto & iexplanation = static_cast<IntervalExplanation &>(explanation);

    auto & fw = expand.framework;
    auto const maxAttempts = config.maxAttempts;
    assert(maxAttempts > 0);

    std::size_t const varSize = fw.varSize();
    for (VarIdx idxToRelax : varOrdering.manualOrder) {
        auto & optVarBndToRelax = iexplanation.tryGetVarBound(idxToRelax);
        if (not optVarBndToRelax.has_value()) { continue; }

        verifier.push();
        for (VarIdx idx = 0; idx < varSize; ++idx) {
            if (idx == idxToRelax) { continue; }
            auto & optVarBnd = iexplanation.tryGetVarBound(idx);
            if (not optVarBnd.has_value()) { continue; }

            auto & varBnd = *optVarBnd;
            assertVarBound(varBnd);
        }

        auto & varBndToRelax = *optVarBndToRelax;
        Interval origInterval = varBndToRelax.toInterval();
        Interval const & domainInterval = fw.getDomainInterval(idxToRelax);
        auto [oLo, oHi] = origInterval.getBounds();
        auto const [dLo, dHi] = domainInterval.getBounds();
        assert(dLo <= oLo and oHi <= dHi);
        assert(dLo < oLo or oHi < dHi);

        if (oLo != dLo) {
            verifier.push();
            Interval relaxedLowerIval{dLo, oHi};
            for (int i = 0; i < maxAttempts; ++i) {
                Float const lo = relaxedLowerIval.getLower();
                assert(lo < oLo);
                assertInterval(idxToRelax, relaxedLowerIval);
                bool const ok = checkFormsExplanation();
                if (ok) {
                    oLo = lo;
                    origInterval.setLower(oLo);
                    break;
                }
                relaxedLowerIval.setLower((lo + oLo) / 2);
            }
            verifier.pop();
        }

        if (oHi != dHi) {
            verifier.push();
            Interval relaxedUpperIval{oLo, dHi};
            for (int i = 0; i < maxAttempts; ++i) {
                Float const hi = relaxedUpperIval.getUpper();
                assert(hi > oHi);
                assertInterval(idxToRelax, relaxedUpperIval);
                bool const ok = checkFormsExplanation();
                if (ok) {
                    oHi = hi;
                    origInterval.setUpper(oHi);
                    break;
                }
                relaxedUpperIval.setUpper((hi + oHi) / 2);
            }
            verifier.pop();
        }

        verifier.pop();

        auto optVarBnd = intervalToOptVarBound(fw, idxToRelax, std::move(origInterval));
        iexplanation.setVarBound(idxToRelax, std::move(optVarBnd));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Framework::Expand::UnsatCoreStrategy::executeBody(Explanation & explanation) {
    assert(dynamic_cast<xai::verifiers::UnsatCoreVerifier *>(expand.verifierPtr.get()));
    auto & verifier = static_cast<xai::verifiers::UnsatCoreVerifier &>(*expand.verifierPtr);

    assert(dynamic_cast<IntervalExplanation *>(&explanation));
    auto & iexplanation = static_cast<IntervalExplanation &>(explanation);

    auto & fw = expand.framework;
    bool const splitEq = config.splitEq;

    for (VarIdx idx : varOrdering.manualOrder) {
        auto & optVarBnd = iexplanation.tryGetVarBound(idx);
        if (not optVarBnd.has_value()) { continue; }

        auto & varBnd = *optVarBnd;
        assertVarBound(varBnd, splitEq);
    }

    [[maybe_unused]] bool const ok = checkFormsExplanation();
    assert(ok);

    xai::verifiers::UnsatCore unsatCore = verifier.getUnsatCore();

    IntervalExplanation newExplanation{fw};

    for (VarIdx idx : unsatCore.equalities) {
        newExplanation.insertBound(idx, EqBound{iexplanation.tryGetVarBound(idx)->getBound().getValue()});
    }
    for (VarIdx idx : unsatCore.lowerBounds) {
        newExplanation.insertBound(idx, LowerBound{iexplanation.tryGetVarBound(idx)->getBound().getValue()});
    }
    for (VarIdx idx : unsatCore.upperBounds) {
        newExplanation.insertBound(idx, UpperBound{iexplanation.tryGetVarBound(idx)->getBound().getValue()});
    }

    iexplanation.swap(newExplanation);
}
} // namespace xspace
