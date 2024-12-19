#include "UnsatCoreStrategy.h"

#include <xspace/framework/explanation/IntervalExplanation.h>
#include <xspace/framework/explanation/VarBound.h>

#include <verifiers/UnsatCoreVerifier.h>

#include <cassert>

namespace xspace {
xai::verifiers::UnsatCoreVerifier & Framework::Expand::UnsatCoreStrategy::getVerifier() {
    auto & verifier = Strategy::getVerifier();
    assert(dynamic_cast<xai::verifiers::UnsatCoreVerifier *>(&verifier));
    return static_cast<xai::verifiers::UnsatCoreVerifier &>(verifier);
}

void Framework::Expand::UnsatCoreStrategy::executeBody(std::unique_ptr<Explanation> & explanationPtr) {
    auto & verifier = getVerifier();

    auto & explanation = *explanationPtr;
    assert(dynamic_cast<IntervalExplanation *>(&explanation));
    auto & iexplanation = static_cast<IntervalExplanation &>(explanation);

    auto & fw = expand.getFramework();

    assertIntervalExplanation(iexplanation, {.ignoreVarOrder = false, .splitEq = config.splitEq});
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
