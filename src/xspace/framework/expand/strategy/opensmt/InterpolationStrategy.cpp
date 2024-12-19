#include "InterpolationStrategy.h"

#include <xspace/framework/explanation/Explanation.h>
#include <xspace/framework/explanation/IntervalExplanation.h>
#include <xspace/framework/explanation/opensmt/FormulaExplanation.h>

#include <xspace/common/Utils.h>

#include <verifiers/opensmt/OpenSMTVerifier.h>

#include <api/MainSolver.h>

#include <cassert>

namespace xspace::expand::opensmt {
xai::verifiers::OpenSMTVerifier & InterpolationStrategy::getVerifier() {
    auto & verifier = Strategy::getVerifier();
    assert(dynamic_cast<xai::verifiers::OpenSMTVerifier *>(&verifier));
    return static_cast<xai::verifiers::OpenSMTVerifier &>(verifier);
}

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
    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();

    auto & fw = expand.getFramework();

    auto & explanation = *explanationPtr;

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

    assignNew<FormulaExplanation>(explanationPtr, fw, itp);
}

bool InterpolationStrategy::assertExplanationImpl(Explanation const & explanation, AssertExplanationConf const & conf) {
    if (Strategy::assertExplanationImpl(explanation, conf)) { return true; }

    assert(dynamic_cast<FormulaExplanation const *>(&explanation));
    auto & phiexplanation = static_cast<FormulaExplanation const &>(explanation);
    assertFormulaExplanation(phiexplanation, conf);

    return true;
}

void InterpolationStrategy::assertFormulaExplanation(FormulaExplanation const & phiexplanation) {
    assertFormulaExplanation(phiexplanation, AssertExplanationConf{});
}

void InterpolationStrategy::assertFormulaExplanation(FormulaExplanation const & phiexplanation,
                                                     AssertExplanationConf const &) {
    Formula const & phi = phiexplanation.getFormula();

    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();
    solver.insertFormula(phi);
}
} // namespace xspace::expand::opensmt
