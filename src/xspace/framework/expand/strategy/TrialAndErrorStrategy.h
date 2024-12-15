#ifndef XSPACE_EXPAND_TRIALSTRATEGY_H
#define XSPACE_EXPAND_TRIALSTRATEGY_H

#include "Strategy.h"

namespace xspace {
class Framework::Expand::TrialAndErrorStrategy : public Strategy {
public:
    struct Config {
        int maxAttempts = 4;
    };

    using Strategy::Strategy;
    TrialAndErrorStrategy(Expand & exp, Config const & conf, VarOrdering order = {})
        : Strategy{exp, std::move(order)},
          config{conf} {}

    bool isAbductiveOnly() const override { return false; }

protected:
    void executeBody(std::unique_ptr<Explanation> &) override;

    Config config{};
};
} // namespace xspace

#endif // XSPACE_EXPAND_TRIALSTRATEGY_H
