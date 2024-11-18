#ifndef XSPACE_EXPAND_STRATEGY_H
#define XSPACE_EXPAND_STRATEGY_H

#include "../Expand.h"

namespace xspace {
class IntervalExplanation;

class Framework::Expand::Strategy {
public:
    Strategy(Expand &, VarOrdering = {});
    virtual ~Strategy() = default;

    virtual void execute(IntervalExplanation &);

protected:
    virtual void executeInit(IntervalExplanation &);
    virtual void executeBody(IntervalExplanation &) = 0;
    virtual void executeFinish(IntervalExplanation &) {}

    Expand & expand;

    VarOrdering varOrdering;
};

class Framework::Expand::AbductiveStrategy : public Strategy {
public:
    using Strategy::Strategy;

protected:
    void executeBody(IntervalExplanation &) override;
};
} // namespace xspace

#endif // XSPACE_EXPAND_STRATEGY_H
