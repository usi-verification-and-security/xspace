#ifndef XSPACE_EXPAND_OSMTUCORESTRATEGY_H
#define XSPACE_EXPAND_OSMTUCORESTRATEGY_H

#include "Strategy.h"
#include "../UnsatCoreStrategy.h"

namespace xspace::expand::opensmt {
class UnsatCoreStrategy : public Strategy, virtual public Framework::Expand::UnsatCoreStrategy {
public:
    using Framework::Expand::UnsatCoreStrategy::Config;

    using Strategy::Strategy;
    UnsatCoreStrategy(Framework::Expand & exp, Config const & conf, Framework::Expand::VarOrdering order = {})
        : Framework::Expand::Strategy{exp, std::move(order)},
          Framework::Expand::UnsatCoreStrategy{exp, conf} {}

protected:
    using Strategy::getVerifier;

    void executeBody(std::unique_ptr<Explanation> &) override;
};
} // namespace xspace::expand::opensmt

#endif // XSPACE_EXPAND_OSMTUCORESTRATEGY_H
