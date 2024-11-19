#include "Strategy.h"

#include <xspace/explanation/Explanation.h>

#include <verifiers/Verifier.h>

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
    auto const & varOrder = varOrdering.manualOrder;
    for (VarIdx idxToOmit : varOrder) {
        verifier.push();
        for (VarIdx idx = 0; idx < varSize; ++idx) {
            if (idx == idxToOmit) { continue; }
            auto & optVarBnd = explanation.tryGetVarBound(idx);
            if (not optVarBnd.has_value()) { continue; }

            auto & varBnd = *optVarBnd;
            assert(not varBnd.isInterval());
            auto & bnd = varBnd.getBound();
            expand.assertBound(idx, bnd);
        }
        bool const ok = expand.checkFormsExplanation();
        verifier.pop();
        verifier.resetSample();
        // It is no longer explanation after the removal -> we cannot remove it
        if (not ok) { continue; }
        explanation.eraseVarBound(idxToOmit);
    }
}
} // namespace xspace
