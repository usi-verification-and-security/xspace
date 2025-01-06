#include "Strategy.h"

#include <xspace/framework/explanation/opensmt/FormulaExplanation.h>

#include <verifiers/opensmt/OpenSMTVerifier.h>

#include <api/MainSolver.h>

#include <cassert>

namespace xspace::expand::opensmt {
xai::verifiers::OpenSMTVerifier & Strategy::getVerifier() {
    auto & verifier = Framework::Expand::Strategy::getVerifier();
    assert(dynamic_cast<xai::verifiers::OpenSMTVerifier *>(&verifier));
    return static_cast<xai::verifiers::OpenSMTVerifier &>(verifier);
}

bool Strategy::assertExplanationImpl(PartialExplanation const & pexplanation, AssertExplanationConf const & conf) {
    if (Framework::Expand::Strategy::assertExplanationImpl(pexplanation, conf)) { return true; }

    assert(dynamic_cast<FormulaExplanation const *>(&pexplanation));
    auto & phiexplanation = static_cast<FormulaExplanation const &>(pexplanation);
    assertFormulaExplanation(phiexplanation, conf);

    return true;
}

void Strategy::assertFormulaExplanation(FormulaExplanation const & phiexplanation) {
    assertFormulaExplanation(phiexplanation, AssertExplanationConf{});
}

void Strategy::assertFormulaExplanation(FormulaExplanation const & phiexplanation, AssertExplanationConf const &) {
    Formula const & phi = phiexplanation.getFormula();

    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();
    solver.insertFormula(phi);
}
} // namespace xspace::expand::opensmt
