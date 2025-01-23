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

std::size_t FormulaExplanation::termSize() const {
    return termSizeOf(*formulaPtr);
}

std::size_t FormulaExplanation::termSizeOf(Formula const & phi) const {
    auto & solver = getVerifier().getSolver();
    auto & logic = solver.getLogic();
    auto & phiTerm = logic.getPterm(phi);

    if (logic.isAtom(phi)) { return 1; }

    if (logic.isNot(phi)) {
        assert(phiTerm.size() == 1);
        auto & negPhi = *phiTerm.begin();
        assert(not logic.isNot(negPhi));
        // just care about no. literals, so ignore the negations themselves
        return termSizeOf(negPhi);
    }

    assert(logic.isAnd(phi) or logic.isOr(phi));
    std::size_t totalSize{};
    for (Formula const & argPhi : phiTerm) {
        totalSize += termSizeOf(argPhi);
    }
    return totalSize;
}

void FormulaExplanation::clear() {
    Explanation::clear();

    resetFormula();
}

void FormulaExplanation::resetFormula() {
    formulaPtr.reset();
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
