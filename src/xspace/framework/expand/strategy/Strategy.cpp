#include "Strategy.h"

#include <xspace/explanation/Explanation.h>

#include <verifiers/Verifier.h>
#include <verifiers/UnsatCoreVerifier.h>

#include <cassert>
#include <numeric>

namespace xspace {
Framework::Expand::Strategy::Strategy(Expand & exp, VarOrdering order) : expand{exp}, varOrdering{std::move(order)} {
    assert((varOrdering.type == VarOrdering::Type::manual) xor varOrdering.manualOrder.empty());
}

void Framework::Expand::Strategy::execute(IntervalExplanation & explanation) {
    executeInit(explanation);
    executeBody(explanation);
    executeFinish(explanation);
}

void Framework::Expand::Strategy::executeInit(IntervalExplanation &) {
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

////////////////////////////////////////////////////////////////////////////////////////////////////

void Framework::Expand::AbductiveStrategy::executeBody(IntervalExplanation & explanation) {
    auto & verifier = *expand.verifierPtr;

    std::size_t const varSize = expand.framework.varSize();
    for (VarIdx idxToOmit : varOrdering.manualOrder) {
        verifier.push();
        for (VarIdx idx = 0; idx < varSize; ++idx) {
            if (idx == idxToOmit) { continue; }
            auto & optVarBnd = explanation.tryGetVarBound(idx);
            if (not optVarBnd.has_value()) { continue; }

            auto & varBnd = *optVarBnd;
            expand.assertVarBound(idx, varBnd);
        }
        bool const ok = expand.checkFormsExplanation();
        verifier.pop();
        // It is no longer explanation after the removal -> we cannot remove it
        if (not ok) { continue; }
        explanation.eraseVarBound(idxToOmit);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Framework::Expand::UnsatCoreStrategy::executeBody(IntervalExplanation & explanation) {
    assert(dynamic_cast<xai::verifiers::UnsatCoreVerifier *>(expand.verifierPtr.get()));
    auto & verifier = static_cast<xai::verifiers::UnsatCoreVerifier &>(*expand.verifierPtr);

    auto & fw = expand.framework;
    bool const splitEq = config.splitEq;

    for (VarIdx idx : varOrdering.manualOrder) {
        auto & optVarBnd = explanation.tryGetVarBound(idx);
        if (not optVarBnd.has_value()) { continue; }

        auto & varBnd = *optVarBnd;
        expand.assertVarBound(idx, varBnd, splitEq);
    }

    [[maybe_unused]] bool const ok = expand.checkFormsExplanation();
    assert(ok);

    xai::verifiers::UnsatCore unsatCore = verifier.getUnsatCore();

    IntervalExplanation newExplanation{fw};

    for (VarIdx idx : unsatCore.equalities) {
        newExplanation.insertBound(idx, EqBound{explanation.tryGetVarBound(idx)->getBound().getValue()});
    }
    for (VarIdx idx : unsatCore.lowerBounds) {
        newExplanation.insertBound(idx, LowerBound{explanation.tryGetVarBound(idx)->getBound().getValue()});
    }
    for (VarIdx idx : unsatCore.upperBounds) {
        newExplanation.insertBound(idx, UpperBound{explanation.tryGetVarBound(idx)->getBound().getValue()});
    }

    explanation.swap(newExplanation);
}
} // namespace xspace
