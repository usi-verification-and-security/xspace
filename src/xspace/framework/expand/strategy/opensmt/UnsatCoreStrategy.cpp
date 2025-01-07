#include "UnsatCoreStrategy.h"

#include <xspace/framework/explanation/ConjunctExplanation.h>
#include <xspace/framework/explanation/IntervalExplanation.h>
#include <xspace/framework/explanation/opensmt/FormulaExplanation.h>

#include <xspace/common/Utils.h>

#include <verifiers/opensmt/OpenSMTVerifier.h>

#include <api/MainSolver.h>

#include <cassert>

namespace xspace::expand::opensmt {
void UnsatCoreStrategy::executeBody(std::unique_ptr<Explanation> & explanationPtr) {
    auto & explanation = *explanationPtr;
    if (dynamic_cast<IntervalExplanation *>(&explanation)) {
        Framework::Expand::UnsatCoreStrategy::executeBody(explanationPtr);
        return;
    }

    if (not dynamic_cast<ConjunctExplanation *>(&explanation)) { return; }
    auto & cexplanation = static_cast<ConjunctExplanation &>(explanation);

    bool const splitEq = config.splitEq;

    assertConjunctExplanation(cexplanation, {.splitEq = splitEq});
    [[maybe_unused]] bool const ok = checkFormsExplanation();
    assert(ok);

    auto & fw = expand.getFramework();
    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();

    auto const unsatCore = solver.getUnsatCore();

    ConjunctExplanation newExplanation{fw};

    //+ erasing instead of inserting would keep the same explanation types
    // .. but in the case of VarBound, more than one term can refer to one expl. (lower/upper bounds)
    for (Formula const & phi : unsatCore->getTerms()) {
        auto phiexplanationPtr = std::make_unique<FormulaExplanation>(fw, phi);
        newExplanation.insertExplanation(std::move(phiexplanationPtr));
    }

    assignNew<ConjunctExplanation>(explanationPtr, std::move(newExplanation));
}
} // namespace xspace::expand::opensmt
