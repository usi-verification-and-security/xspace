#include "Strategy.h"

#include <xspace/framework/explanation/ConjunctExplanation.h>
#include <xspace/framework/explanation/Explanation.h>
#include <xspace/framework/explanation/IntervalExplanation.h>
#include <xspace/framework/explanation/VarBound.h>

#include <verifiers/Verifier.h>

#include <cassert>
#include <numeric>

namespace xspace {
Framework::Expand::Strategy::Strategy(Expand & exp, VarOrdering order) : expand{exp}, varOrdering{std::move(order)} {
    assert((varOrdering.type == VarOrdering::Type::manual) xor varOrdering.manualOrder.empty());
}

void Framework::Expand::Strategy::execute(std::unique_ptr<Explanation> & explanationPtr) {
    executeInit(explanationPtr);
    executeBody(explanationPtr);
    executeFinish(explanationPtr);
}

void Framework::Expand::Strategy::executeInit(std::unique_ptr<Explanation> &) {
    initVarOrdering();

    auto & verifier = getVerifier();
    verifier.push();
}

void Framework::Expand::Strategy::executeFinish(std::unique_ptr<Explanation> &) {
    auto & verifier = getVerifier();
    verifier.pop();
}

void Framework::Expand::Strategy::initVarOrdering() {
    auto const & orderType = varOrdering.type;
    auto & varOrder = varOrdering.manualOrder;
    std::size_t const varSize = expand.getFramework().varSize();
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

    //? sort the variables right away, and then sort back at the end
}

void Framework::Expand::Strategy::assertExplanation(PartialExplanation const & pexplanation) {
    assertExplanation(pexplanation, AssertExplanationConf{});
}

void Framework::Expand::Strategy::assertExplanation(PartialExplanation const & pexplanation,
                                                    AssertExplanationConf const & conf) {
    [[maybe_unused]] bool success = assertExplanationImpl(pexplanation, conf);
    assert(success);
}

bool Framework::Expand::Strategy::assertExplanationImpl(PartialExplanation const & pexplanation,
                                                        AssertExplanationConf const & conf) {
    // Not necessary yet
    assert(not dynamic_cast<VarBound const *>(&pexplanation));
    /*
    if (auto * varBndPtr = dynamic_cast<VarBound const *>(&pexplanation)) {
        assertVarBound(*varBndPtr, conf);
        return true;
    }
    */

    if (auto * cexpPtr = dynamic_cast<ConjunctExplanation const *>(&pexplanation)) {
        assertConjunctExplanation(*cexpPtr, conf);
        return true;
    }

    return false;
}

void Framework::Expand::Strategy::assertConjunctExplanation(ConjunctExplanation const & cexplanation) {
    assertConjunctExplanation(cexplanation, AssertExplanationConf{});
}

void Framework::Expand::Strategy::assertConjunctExplanation(ConjunctExplanation const & cexplanation,
                                                            AssertExplanationConf const & conf) {
    if (auto * iexpPtr = dynamic_cast<IntervalExplanation const *>(&cexplanation)) {
        assertIntervalExplanation(*iexpPtr, conf);
        return;
    }

    // Does not consider var ordering, unlike interval explanations
    for (auto & pexplanationPtr : cexplanation) {
        assertExplanation(*pexplanationPtr, conf);
    }
}

void Framework::Expand::Strategy::assertIntervalExplanation(IntervalExplanation const & iexplanation) {
    assertIntervalExplanation(iexplanation, AssertExplanationConf{});
}

void Framework::Expand::Strategy::assertIntervalExplanation(IntervalExplanation const & iexplanation,
                                                            AssertExplanationConf const & conf) {
    assertIntervalExplanationTp(iexplanation, conf);
}

void Framework::Expand::Strategy::assertIntervalExplanationExcept(IntervalExplanation const & iexplanation,
                                                                  VarIdx idxToOmit) {
    assertIntervalExplanationExcept(iexplanation, idxToOmit, AssertExplanationConf{});
}

void Framework::Expand::Strategy::assertIntervalExplanationExcept(IntervalExplanation const & iexplanation,
                                                                  VarIdx idxToOmit,
                                                                  AssertExplanationConf const & conf) {
    assertIntervalExplanationTp<true>(iexplanation, conf, idxToOmit);
}

template<bool omitIdx>
void Framework::Expand::Strategy::assertIntervalExplanationTp(IntervalExplanation const & iexplanation,
                                                              AssertExplanationConf const & conf,
                                                              [[maybe_unused]] VarIdx idxToOmit) {
    if (conf.ignoreVarOrder) {
        std::size_t const esize = iexplanation.size();
        for (VarIdx idx = 0; idx < esize; ++idx) {
            if constexpr (omitIdx) {
                if (idx == idxToOmit) { continue; }
            } else {
                assert(idx != idxToOmit);
            }

            auto * optVarBnd = iexplanation.tryGetVarBound(idx);
            if (not optVarBnd) { continue; }

            auto & varBnd = *optVarBnd;
            assertVarBound(varBnd, conf);
        }
        return;
    }

    for (VarIdx idx : varOrdering.manualOrder) {
        if constexpr (omitIdx) {
            if (idx == idxToOmit) { continue; }
        } else {
            assert(idx != idxToOmit);
        }

        auto * optVarBnd = iexplanation.tryGetVarBound(idx);
        if (not optVarBnd) { continue; }

        auto & varBnd = *optVarBnd;
        assertVarBound(varBnd, conf);
    }
}

void Framework::Expand::Strategy::assertVarBound(VarBound const & varBnd) {
    assertVarBound(varBnd, AssertExplanationConf{});
}

void Framework::Expand::Strategy::assertVarBound(VarBound const & varBnd, AssertExplanationConf const & conf) {
    VarIdx const idx = varBnd.getVarIdx();
    if (varBnd.isInterval()) {
        assertInnerInterval(idx, varBnd.getIntervalLower(), varBnd.getIntervalUpper());
        return;
    }

    if (varBnd.isPoint()) {
        assertPoint(idx, varBnd.getPoint(), conf.splitEq);
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
    auto & network = expand.getFramework().getNetwork();
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
    auto & network = expand.getFramework().getNetwork();
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
    assert(val >= expand.getFramework().getNetwork().getInputLowerBound(idx));
    assert(val <= expand.getFramework().getNetwork().getInputUpperBound(idx));
    getVerifier().addEquality(0, idx, val, storeNamedTerms());
}

void Framework::Expand::Strategy::assertLowerBound(VarIdx idx, LowerBound const & lo) {
    Float const val = lo.getValue();
    assert(val > expand.getFramework().getNetwork().getInputLowerBound(idx));
    assert(val < expand.getFramework().getNetwork().getInputUpperBound(idx));
    getVerifier().addLowerBound(0, idx, val, storeNamedTerms());
}

void Framework::Expand::Strategy::assertUpperBound(VarIdx idx, UpperBound const & hi) {
    Float const val = hi.getValue();
    assert(val > expand.getFramework().getNetwork().getInputLowerBound(idx));
    assert(val < expand.getFramework().getNetwork().getInputUpperBound(idx));
    getVerifier().addUpperBound(0, idx, val, storeNamedTerms());
}

bool Framework::Expand::Strategy::checkFormsExplanation() {
    auto answer = getVerifier().check();
    assert(answer == xai::verifiers::Verifier::Answer::SAT or answer == xai::verifiers::Verifier::Answer::UNSAT);
    return (answer == xai::verifiers::Verifier::Answer::UNSAT);
}
} // namespace xspace
