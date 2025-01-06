#include "InterpolationStrategy.h"

#include <xspace/framework/explanation/Explanation.h>
#include <xspace/framework/explanation/opensmt/FormulaExplanation.h>

#include <xspace/common/Utils.h>

#include <verifiers/opensmt/OpenSMTVerifier.h>

#include <api/MainSolver.h>

#include <cassert>

namespace xspace::expand::opensmt {
void InterpolationStrategy::executeInit(std::unique_ptr<Explanation> &) {
    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();
    auto & solverConf = solver.getConfig();

    switch (config.boolInterpolationAlg) {
        case BoolInterpolationAlg::weak:
            solverConf.setBooleanInterpolationAlgorithm(::opensmt::itp_alg_mcmillanp);
            break;
        case BoolInterpolationAlg::strong:
            solverConf.setBooleanInterpolationAlgorithm(::opensmt::itp_alg_mcmillan);
            break;
    }

    switch (config.arithInterpolationAlg) {
        case ArithInterpolationAlg::weak:
            solverConf.setLRAInterpolationAlgorithm(::opensmt::itp_lra_alg_weak);
            break;
        case ArithInterpolationAlg::weaker:
            solverConf.setLRAInterpolationAlgorithm(::opensmt::itp_lra_alg_decomposing_weak);
            break;
        case ArithInterpolationAlg::strong:
            solverConf.setLRAInterpolationAlgorithm(::opensmt::itp_lra_alg_strong);
            break;
        case ArithInterpolationAlg::stronger:
            solverConf.setLRAInterpolationAlgorithm(::opensmt::itp_lra_alg_decomposing_strong); // seems too strong ...
            break;
    }
}

void InterpolationStrategy::executeBody(std::unique_ptr<Explanation> & explanationPtr) {
    auto & explanation = *explanationPtr;

    auto & fw = expand.getFramework();
    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();

    auto const firstIdx = solver.getInsertedFormulasCount();
    assertExplanation(explanation);
    auto const lastIdx = solver.getInsertedFormulasCount() - 1;

    [[maybe_unused]] bool const ok = checkFormsExplanation();
    assert(ok);

    ::opensmt::ipartitions_t part = 0;
    for (auto idx = firstIdx; idx <= lastIdx; ++idx) {
        ::opensmt::setbit(part, idx);
    }

    ::opensmt::vec<Formula> itps;
    auto interpolationContext = solver.getInterpolationContext();
    interpolationContext->getSingleInterpolant(itps, part);
    assert(itps.size() == 1);
    Formula itp = itps[0];

    //++ sometimes may produce conjunct explanations
    assignNew<FormulaExplanation>(explanationPtr, fw, itp);
}
} // namespace xspace::expand::opensmt
