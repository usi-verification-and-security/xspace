#include "TrialAndErrorStrategy.h"

#include <xspace/framework/Utils.h>
#include <xspace/framework/explanation/IntervalExplanation.h>
#include <xspace/framework/explanation/VarBound.h>

#include <verifiers/Verifier.h>

#include <cassert>

namespace xspace {
void Framework::Expand::TrialAndErrorStrategy::executeBody(std::unique_ptr<Explanation> & explanationPtr) {
    auto & verifier = *expand.verifierPtr;

    auto & explanation = *explanationPtr;
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
} // namespace xspace
