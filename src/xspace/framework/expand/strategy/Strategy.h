#ifndef XSPACE_EXPAND_STRATEGY_H
#define XSPACE_EXPAND_STRATEGY_H

#include "../Expand.h"

#include <xspace/common/Bound.h>
#include <xspace/common/Interval.h>

namespace xspace {
class VarBound;
class PartialExplanation;
class Explanation;
class ConjunctExplanation;
class IntervalExplanation;

class Framework::Expand::Strategy {
public:
    class Factory;

    Strategy(Expand &, VarOrdering = {});
    virtual ~Strategy() = default;

    static char const * name() = delete;

    virtual bool isAbductiveOnly() const { return true; }

    virtual void execute(std::unique_ptr<Explanation> &);

protected:
    struct AssertExplanationConf {
        bool ignoreVarOrder = true;
        bool splitEq = false;
    };

    xai::verifiers::Verifier & getVerifier() { return *expand.verifierPtr; }

    virtual bool storeNamedTerms() const { return false; }

    virtual void executeInit(std::unique_ptr<Explanation> &);
    virtual void executeBody(std::unique_ptr<Explanation> &) = 0;
    virtual void executeFinish(std::unique_ptr<Explanation> &);

    void initVarOrdering();

    void assertExplanation(PartialExplanation const &);
    void assertExplanation(PartialExplanation const &, AssertExplanationConf const &);
    virtual bool assertExplanationImpl(PartialExplanation const &, AssertExplanationConf const &);

    void assertConjunctExplanation(ConjunctExplanation const &);
    void assertConjunctExplanation(ConjunctExplanation const &, AssertExplanationConf const &);

    void assertIntervalExplanation(IntervalExplanation const &);
    void assertIntervalExplanation(IntervalExplanation const &, AssertExplanationConf const &);
    void assertIntervalExplanationExcept(IntervalExplanation const &, VarIdx);
    void assertIntervalExplanationExcept(IntervalExplanation const &, VarIdx, AssertExplanationConf const &);

    void assertVarBound(VarBound const &);
    void assertVarBound(VarBound const &, AssertExplanationConf const &);
    void assertInterval(VarIdx, Interval const &);
    void assertInnerInterval(VarIdx, Interval const &);
    void assertInnerInterval(VarIdx, LowerBound const &, UpperBound const &);
    void assertPoint(VarIdx, Float);
    void assertPoint(VarIdx, EqBound const &, bool splitEq);
    void assertPointNoSplit(VarIdx, EqBound const &);
    void assertPointSplit(VarIdx, EqBound const &);

    void assertBound(VarIdx, Bound const &);
    void assertEquality(VarIdx, EqBound const &);
    void assertLowerBound(VarIdx, LowerBound const &);
    void assertUpperBound(VarIdx, UpperBound const &);

    bool checkFormsExplanation();

    Expand & expand;

    VarOrdering varOrdering;

private:
    template<bool omitIdx = false>
    void assertIntervalExplanationTp(IntervalExplanation const &, AssertExplanationConf const &,
                                     VarIdx idxToOmit = invalidVarIdx);
};
} // namespace xspace

#endif // XSPACE_EXPAND_STRATEGY_H
