#ifndef XSPACE_EXPAND_STRATEGY_H
#define XSPACE_EXPAND_STRATEGY_H

#include "../Expand.h"

#include <xspace/common/Bound.h>
#include <xspace/common/Interval.h>

namespace xspace {
class VarBound;
class Explanation;

class Framework::Expand::Strategy {
public:
    Strategy(Expand &, VarOrdering = {});
    virtual ~Strategy() = default;

    virtual bool isAbductiveOnly() const { return true; }

    virtual void execute(std::unique_ptr<Explanation> &);

protected:
    virtual bool storeNamedTerms() const { return false; }

    virtual void executeInit(std::unique_ptr<Explanation> &);
    virtual void executeBody(std::unique_ptr<Explanation> &) = 0;
    virtual void executeFinish(std::unique_ptr<Explanation> &) {}

    void assertVarBound(VarBound const &, bool splitEq = false);
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
};
} // namespace xspace

#endif // XSPACE_EXPAND_STRATEGY_H
