#include "Strategy.h"

#include <xspace/framework/explanation/opensmt/FormulaExplanation.h>

#include <verifiers/opensmt/OpenSMTVerifier.h>

#include <api/MainSolver.h>

#include <cassert>

namespace xspace::expand::opensmt {
xai::verifiers::OpenSMTVerifier const & Strategy::getVerifier() const {
    auto & verifier = Framework::Expand::Strategy::getVerifier();
    assert(dynamic_cast<xai::verifiers::OpenSMTVerifier const *>(&verifier));
    return static_cast<xai::verifiers::OpenSMTVerifier const &>(verifier);
}

xai::verifiers::OpenSMTVerifier & Strategy::getVerifier() {
    return const_cast<xai::verifiers::OpenSMTVerifier &>(std::as_const(*this).getVerifier());
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

void Strategy::assertFormulaExplanation(FormulaExplanation const & phiexplanation, [[maybe_unused]] AssertExplanationConf const & conf) {
    assert(not conf.splitIntervals);

    static std::size_t counter{};

    Formula const & phi = phiexplanation.getFormula();

    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();
    solver.insertFormula(phi);
    if (not storeNamedTerms()) { return; }

    solver.getTermNames().insert("phi_"s + std::to_string(counter++), phi);
}
} // namespace xspace::expand::opensmt
