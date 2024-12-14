#include "AbductiveStrategy.h"

#include <xspace/framework/explanation/IntervalExplanation.h>
#include <xspace/framework/explanation/VarBound.h>

#include <verifiers/Verifier.h>

#include <cassert>

namespace xspace {
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
} // namespace xspace
