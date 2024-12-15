#include "AbductiveStrategy.h"

#include <xspace/framework/explanation/IntervalExplanation.h>
#include <xspace/framework/explanation/VarBound.h>

#include <verifiers/Verifier.h>

#include <cassert>

namespace xspace {
void Framework::Expand::AbductiveStrategy::executeBody(std::unique_ptr<Explanation> & explanationPtr) {
    auto & verifier = *expand.verifierPtr;

    auto & explanation = *explanationPtr;
    assert(dynamic_cast<IntervalExplanation *>(&explanation));
    auto & iexplanation = static_cast<IntervalExplanation &>(explanation);

    for (VarIdx idxToOmit : varOrdering.manualOrder) {
        verifier.push();
        assertIntervalExplanationExcept(iexplanation, idxToOmit);
        bool const ok = checkFormsExplanation();
        verifier.pop();
        // It is no longer explanation after the removal -> we cannot remove it
        if (not ok) { continue; }
        iexplanation.eraseVarBound(idxToOmit);
    }
}
} // namespace xspace
