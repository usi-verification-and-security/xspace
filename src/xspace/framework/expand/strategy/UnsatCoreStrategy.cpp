#include "UnsatCoreStrategy.h"

#include <xspace/framework/explanation/IntervalExplanation.h>
#include <xspace/framework/explanation/VarBound.h>

#include <verifiers/UnsatCoreVerifier.h>

#include <cassert>

namespace xspace {
void Framework::Expand::UnsatCoreStrategy::executeBody(std::unique_ptr<Explanation> & explanationPtr) {
    assert(dynamic_cast<xai::verifiers::UnsatCoreVerifier *>(expand.verifierPtr.get()));
    auto & verifier = static_cast<xai::verifiers::UnsatCoreVerifier &>(*expand.verifierPtr);

    auto & explanation = *explanationPtr;
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
