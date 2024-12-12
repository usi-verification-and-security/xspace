#ifndef XSPACE_EXPAND_STRATEGY_H
#define XSPACE_EXPAND_STRATEGY_H

#include "../Expand.h"

#include <xspace/common/Bound.h>
#include <xspace/common/Interval.h>

namespace xspace {
class VarBound;
class IntervalExplanation;

class Framework::Expand::Strategy {
public:
    Strategy(Expand &, VarOrdering = {});
    virtual ~Strategy() = default;

    virtual bool isAbductiveOnly() const { return true; }

    virtual void execute(IntervalExplanation &);

protected:
    virtual void executeInit(IntervalExplanation &);
    virtual void executeBody(IntervalExplanation &) = 0;
    virtual void executeFinish(IntervalExplanation &) {}

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

class Framework::Expand::AbductiveStrategy : public Strategy {
public:
    using Strategy::Strategy;

protected:
    void executeBody(IntervalExplanation &) override;
};

class Framework::Expand::UnsatCoreStrategy : public Strategy {
public:
    struct Config {
        bool splitEq = false;
    };

    using Strategy::Strategy;
    UnsatCoreStrategy(Expand & exp, Config const & conf, VarOrdering order = {})
        : Strategy{exp, std::move(order)},
          config{conf} {}

    bool isAbductiveOnly() const override { return not config.splitEq; }

protected:
    void executeBody(IntervalExplanation &) override;

    Config config{};
};
} // namespace xspace

#endif // XSPACE_EXPAND_STRATEGY_H
