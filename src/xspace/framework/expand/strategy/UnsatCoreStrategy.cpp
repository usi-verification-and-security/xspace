#include "UnsatCoreStrategy.h"

#include <xspace/framework/explanation/ConjunctExplanation.h>
#include <xspace/framework/explanation/IntervalExplanation.h>
#include <xspace/framework/explanation/VarBound.h>

#include <verifiers/UnsatCoreVerifier.h>

#include <cassert>

#ifndef NDEBUG
#include <algorithm>
#endif

namespace xspace {
xai::verifiers::UnsatCoreVerifier const & Framework::Expand::UnsatCoreStrategy::getVerifier() const {
    auto & verifier = Strategy::getVerifier();
    assert(dynamic_cast<xai::verifiers::UnsatCoreVerifier const *>(&verifier));
    return static_cast<xai::verifiers::UnsatCoreVerifier const &>(verifier);
}

xai::verifiers::UnsatCoreVerifier & Framework::Expand::UnsatCoreStrategy::getVerifier() {
    return const_cast<xai::verifiers::UnsatCoreVerifier &>(std::as_const(*this).getVerifier());
}

void Framework::Expand::UnsatCoreStrategy::executeBody(std::unique_ptr<Explanation> & explanationPtr) {
    assert(storeNamedTerms());

    auto & explanation = *explanationPtr;
    if (not dynamic_cast<ConjunctExplanation *>(&explanation)) { return; }

    auto & cexplanation = static_cast<ConjunctExplanation &>(explanation);

    if (auto * iexpPtr = dynamic_cast<IntervalExplanation *>(&cexplanation)) {
        executeBody(*iexpPtr);
        return;
    }

    executeBody(cexplanation);
}

void Framework::Expand::UnsatCoreStrategy::executeBody(ConjunctExplanation & cexplanation) {
    assert(not dynamic_cast<IntervalExplanation *>(&cexplanation));
    //+ not supported for general conjunctions
    assert(not config.splitIntervals);

    // In order to map assertions to explanations, we need the conjunction not to be sparse
    cexplanation.condense();
    assertConjunctExplanation(cexplanation, {.ignoreVarOrder = true, .splitIntervals = false});
    [[maybe_unused]] bool const ok = checkFormsExplanation();
    assert(ok);

    auto & verifier = getVerifier();

    xai::verifiers::UnsatCore unsatCore = verifier.getUnsatCore();
    assert(cexplanation.validSize() == unsatCore.includedIndices.size() + unsatCore.excludedIndices.size());

    // Every particular assertion corresponds to one explanation of the conjunction
    assert(not cexplanation.isSparse());
    for (std::size_t excludedIdx : unsatCore.excludedIndices) {
        [[maybe_unused]] bool const erased = cexplanation.eraseExplanation(excludedIdx);
        assert(erased);
    }

#ifndef NDEBUG
    assert(cexplanation.validSize() == unsatCore.includedIndices.size());
    assert(std::ranges::is_sorted(unsatCore.includedIndices));
    assert(std::ranges::is_sorted(unsatCore.excludedIndices));
    auto includedIt = unsatCore.includedIndices.begin();
    auto excludedIt = unsatCore.excludedIndices.begin();
    for (std::size_t idx = 0; idx < cexplanation.size(); ++idx) {
        auto * optExp = cexplanation.tryGetExplanation(idx);
        if (includedIt != unsatCore.includedIndices.end() and *includedIt == idx) {
            assert(optExp);
            ++includedIt;
            continue;
        }
        if (excludedIt != unsatCore.excludedIndices.end() and *excludedIt == idx) {
            assert(not optExp);
            ++excludedIt;
            continue;
        }
        assert(false);
    }
    assert(includedIt == unsatCore.includedIndices.end());
    assert(excludedIt == unsatCore.excludedIndices.end());
#endif
}

void Framework::Expand::UnsatCoreStrategy::executeBody(IntervalExplanation & iexplanation) {
    bool const splitIntervals = config.splitIntervals;

    assertIntervalExplanation(iexplanation, {.ignoreVarOrder = false, .splitIntervals = splitIntervals});
    [[maybe_unused]] bool const ok = checkFormsExplanation();
    assert(ok);

    auto & fw = expand.getFramework();
    auto & verifier = getVerifier();

    xai::verifiers::UnsatCore unsatCore = verifier.getUnsatCore();

    IntervalExplanation newExplanation{fw};

    for (VarIdx idx : unsatCore.lowerBounds) {
        auto * optVarBnd = iexplanation.tryGetVarBound(idx);
        assert(optVarBnd);
        assert(not optVarBnd->isInterval());
        auto & bnd = optVarBnd->getBound();
        newExplanation.insertBound(idx, LowerBound{std::move(bnd)});
    }
    for (VarIdx idx : unsatCore.upperBounds) {
        auto * optVarBnd = iexplanation.tryGetVarBound(idx);
        assert(optVarBnd);
        assert(not optVarBnd->isInterval());
        auto & bnd = optVarBnd->getBound();
        newExplanation.insertBound(idx, UpperBound{std::move(bnd)});
    }

    for (auto & indices : {unsatCore.equalities, unsatCore.intervals}) {
        for (VarIdx idx : indices) {
            auto * optVarBnd = iexplanation.tryGetVarBound(idx);
            assert(optVarBnd);
            assert(optVarBnd->isPoint() or optVarBnd->isInterval());
            newExplanation.insertVarBound(std::move(*optVarBnd));
        }
    }

    iexplanation.swap(newExplanation);
}
} // namespace xspace
