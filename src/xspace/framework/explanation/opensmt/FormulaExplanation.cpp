#include "FormulaExplanation.h"

#include <xspace/framework/expand/Expand.h>
#include <xspace/framework/expand/strategy/Strategy.h>

#include <xspace/common/Macro.h>

#include <verifiers/opensmt/OpenSMTVerifier.h>

#include <api/MainSolver.h>

#include <cassert>

#ifndef NDEBUG
#include <algorithm>
#endif

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

    assert(not logic.isXor(phi));
    assert(not logic.isImplies(phi));
    assert(not logic.isIff(phi));
    if (logic.isAnd(phi) or logic.isOr(phi)) {
        assert(std::ranges::none_of(phiTerm,
                                    [&logic](Formula const & arg) { return logic.isAnd(arg) or logic.isOr(arg); }));
        return phiTerm.size();
    }

    if (not logic.isNot(phi)) { return 1; }

    assert(phiTerm.size() == 1);
    auto & negPhi = *phiTerm.begin();
    assert(not logic.isNot(negPhi));
    return termSizeOf(negPhi) + 1;
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
