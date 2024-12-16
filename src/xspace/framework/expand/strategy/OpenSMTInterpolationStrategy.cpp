#include "OpenSMTInterpolationStrategy.h"

#include <xspace/framework/explanation/Explanation.h>
#include <xspace/framework/explanation/IntervalExplanation.h>
#include <xspace/framework/explanation/opensmt/FormulaExplanation.h>

#include <xspace/common/Utils.h>

#include <verifiers/opensmt/OpenSMTVerifier.h>

#include <api/MainSolver.h>

#include <cassert>

namespace xspace {
xai::verifiers::OpenSMTVerifier & Framework::Expand::OpenSMTInterpolationStrategy::getVerifier() {
    assert(dynamic_cast<xai::verifiers::OpenSMTVerifier *>(expand.verifierPtr.get()));
    return static_cast<xai::verifiers::OpenSMTVerifier &>(*expand.verifierPtr);
}

void Framework::Expand::OpenSMTInterpolationStrategy::executeInit(std::unique_ptr<Explanation> &) {
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

void Framework::Expand::OpenSMTInterpolationStrategy::executeBody(std::unique_ptr<Explanation> & explanationPtr) {
    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();

    auto & fw = expand.framework;

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

    ::opensmt::vec<opensmt::Formula> itps;
    auto interpolationContext = solver.getInterpolationContext();
    interpolationContext->getSingleInterpolant(itps, part);
    assert(itps.size() == 1);
    opensmt::Formula itp = itps[0];

    assignNew<opensmt::FormulaExplanation>(explanationPtr, fw, itp);
}

bool Framework::Expand::OpenSMTInterpolationStrategy::assertExplanationImpl(Explanation const & explanation,
                                                                            AssertExplanationConf const & conf) {
    if (Strategy::assertExplanationImpl(explanation, conf)) { return true; }

    assert(dynamic_cast<opensmt::FormulaExplanation const *>(&explanation));
    auto & phiexplanation = static_cast<opensmt::FormulaExplanation const &>(explanation);
    assertFormulaExplanation(phiexplanation, conf);

    return true;
}

void Framework::Expand::OpenSMTInterpolationStrategy::assertFormulaExplanation(
    opensmt::FormulaExplanation const & phiexplanation) {
    assertFormulaExplanation(phiexplanation, AssertExplanationConf{});
}

void Framework::Expand::OpenSMTInterpolationStrategy::assertFormulaExplanation(
    opensmt::FormulaExplanation const & phiexplanation, AssertExplanationConf const &) {
    opensmt::Formula const & phi = phiexplanation.getFormula();

    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();
    solver.insertFormula(phi);
}
} // namespace xspace
