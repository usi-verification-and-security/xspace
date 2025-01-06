#include "FormulaExplanation.h"

#include <xspace/framework/expand/Expand.h>
#include <xspace/framework/expand/strategy/Strategy.h>

#include <xspace/common/Macro.h>

#include <verifiers/opensmt/OpenSMTVerifier.h>

#include <api/MainSolver.h>

#include <cassert>

namespace xspace::opensmt {
FormulaExplanation::FormulaExplanation(Framework const & fw, Formula const & phi)
    : Explanation{fw},
      formulaPtr{MAKE_UNIQUE(phi)} {}

xai::verifiers::OpenSMTVerifier const & FormulaExplanation::getVerifier() const {
    assert(dynamic_cast<xai::verifiers::OpenSMTVerifier const *>(&getExpand().getVerifier()));
    return static_cast<xai::verifiers::OpenSMTVerifier const &>(getExpand().getVerifier());
}

bool FormulaExplanation::contains(VarIdx idx) const {
    auto & solver = getVerifier().getSolver();
    auto & logic = solver.getLogic();

    auto & varName = frameworkPtr->getVarName(idx);
    Formula const varPhi = logic.resolveTerm(varName.c_str(), {});
    return logic.contains(*formulaPtr, varPhi);
}

void FormulaExplanation::clear() {
    Explanation::clear();

    resetFormula();
}

void FormulaExplanation::swap(FormulaExplanation & rhs) {
    Explanation::swap(rhs);

    std::swap(formulaPtr, rhs.formulaPtr);
}

void FormulaExplanation::printSmtLib2(std::ostream & os) const {
    auto & solver = getVerifier().getSolver();
    auto & logic = solver.getLogic();
    os << logic.printTerm(*formulaPtr);
}
} // namespace xspace::opensmt
