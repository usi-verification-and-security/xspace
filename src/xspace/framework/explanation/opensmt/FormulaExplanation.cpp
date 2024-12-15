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

std::size_t FormulaExplanation::varSize() const {
    std::size_t const varSize_ = frameworkPtr->varSize();
    std::size_t cnt{};
    for (VarIdx idx = 0; idx < varSize_; ++idx) {
        if (contains(idx)) { ++cnt; }
    }
    return cnt;
}

bool FormulaExplanation::contains(VarIdx) const {
    //+ Not implemented
    return true;
}

void FormulaExplanation::clear() {
    Explanation::clear();

    resetFormula();
}

void FormulaExplanation::swap(FormulaExplanation & rhs) {
    Explanation::swap(rhs);

    std::swap(formulaPtr, rhs.formulaPtr);
}

std::size_t FormulaExplanation::computeFixedCount() const {
    //+ Not implemented
    return 0;
}

Float FormulaExplanation::getRelativeVolume() const {
    //+ Not implemented
    return -1;
}
Float FormulaExplanation::getRelativeVolumeSkipFixed() const {
    //+ Not implemented
    return -1;
}

void FormulaExplanation::printSmtLib2(std::ostream & os) const {
    auto & solver = getVerifier().getSolver();
    auto & logic = solver.getLogic();
    os << logic.printTerm(*formulaPtr);
}
} // namespace xspace::opensmt
