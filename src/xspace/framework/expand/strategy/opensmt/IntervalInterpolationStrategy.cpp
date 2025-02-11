#include "IntervalInterpolationStrategy.h"

#include "UnsatCoreStrategy.h"

#include <xspace/framework/explanation/ConjunctExplanation.h>
#include <xspace/framework/explanation/IntervalExplanation.h>

#include <xspace/common/Utils.h>

#include <verifiers/opensmt/OpenSMTVerifier.h>

#include <api/MainSolver.h>
#include <logics/ArithLogic.h>

#include <algorithm>
#include <cassert>

namespace xspace::expand::opensmt {
void IntervalInterpolationStrategy::executeBody(std::unique_ptr<Explanation> & explanationPtr) {
    assert(dynamic_cast<IntervalExplanation *>(explanationPtr.get()));

    assert(config.boolInterpolationAlg == BoolInterpolationAlg::strong);

    auto & varIndicesFilter = config.varIndicesFilter;
    //+ it can also focus only on these selected vars
    assert(varIndicesFilter.empty());

    for (VarIdx idx : varOrdering.order) {
        InterpolationStrategy::executeInit(explanationPtr);
        varIndicesFilter.push_back(idx);
        InterpolationStrategy::executeBody(explanationPtr);
        varIndicesFilter.pop_back();
        InterpolationStrategy::executeFinish(explanationPtr);
    }

    assert(not dynamic_cast<IntervalExplanation *>(explanationPtr.get()));
    assert(dynamic_cast<ConjunctExplanation *>(explanationPtr.get()));
    auto & cexplanation = static_cast<ConjunctExplanation &>(*explanationPtr);
    // Variables were reversed - it always put the itp at the beginning of the phi
    std::ranges::reverse(cexplanation);

    {
        UnsatCoreStrategy ucoreStrategy{expand};
        ucoreStrategy.execute(explanationPtr);
    }

    auto & fw = expand.getFramework();

    IntervalExplanation newIntervalExplanation{fw};

    for (auto && pexplanationPtr : std::move(cexplanation)) {
        if (not pexplanationPtr) { continue; }

        assert(dynamic_cast<FormulaExplanation *>(pexplanationPtr.get()));
        auto && phiExplanation = static_cast<FormulaExplanation &&>(*pexplanationPtr);
        auto const & phi = phiExplanation.getFormula();
        auto [varIdx, bnd] = formulaToBound(phi);
        newIntervalExplanation.insertBound(varIdx, std::move(bnd));
    }

    assignNew<IntervalExplanation>(explanationPtr, std::move(newIntervalExplanation));
}

std::pair<VarIdx, Bound> IntervalInterpolationStrategy::formulaToBound(Formula const & phi) {
    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();
    auto & logic = solver.getLogic();

    Formula phiCp;
    bool neg = false;
    if (not logic.isNot(phi)) {
        phiCp = phi;
    } else {
        auto & phiTerm = logic.getPterm(phi);
        assert(phiTerm.size() == 1);
        phiCp = phiTerm[0];
        neg = true;
    }

    assert(dynamic_cast<::opensmt::ArithLogic *>(&logic));
    auto & arithLogic = static_cast<::opensmt::ArithLogic &>(logic);

    assert(arithLogic.isLeq(phiCp));
    Formula numberPhi = arithLogic.getConstantFromLeq(phiCp);
    Formula varPhi = arithLogic.getTermFromLeq(phiCp);
    assert(arithLogic.isNumConst(numberPhi));
    assert(arithLogic.isNumVar(varPhi) or arithLogic.isTimes(varPhi));

    ::opensmt::Number number = arithLogic.getNumConst(numberPhi);

    bool isLowerBound = true;
    if (arithLogic.isTimes(varPhi)) {
        auto & timesTerm = logic.getPterm(varPhi);
        assert(timesTerm.size() == 2);
        assert(arithLogic.isMinusOne(timesTerm[0]));
        assert(arithLogic.isNumVar(timesTerm[1]));
        varPhi = timesTerm[1];
        isLowerBound = false;
        number.negate();
    }

    //+ inefficient
    VarName const varName = logic.printTerm(varPhi);
    VarIdx const varIdx = getVarIdx(varName);

    if (neg) {
        //! should be strict inequality - use rational approximation for that
        isLowerBound = not isLowerBound;
    }

    Bound::Type const boundType = isLowerBound ? Bound::lowerType : Bound::upperType;
    //! approximation
    Float val = number.get_d();

    return {varIdx, Bound{boundType, val}};
}
} // namespace xspace::expand::opensmt
