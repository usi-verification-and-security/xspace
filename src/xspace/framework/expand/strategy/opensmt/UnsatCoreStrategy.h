#ifndef XSPACE_EXPAND_OSMTUCORESTRATEGY_H
#define XSPACE_EXPAND_OSMTUCORESTRATEGY_H

#include "Strategy.h"
#include "../UnsatCoreStrategy.h"

namespace xspace::expand::opensmt {
class UnsatCoreStrategy : public Framework::Expand::UnsatCoreStrategy, public Strategy {
public:
    using Framework::Expand::UnsatCoreStrategy::Config;

    using Framework::Expand::UnsatCoreStrategy::UnsatCoreStrategy;
    UnsatCoreStrategy(Framework::Expand & exp, Config const & conf, Framework::Expand::VarOrdering order = {})
        : Framework::Expand::Strategy{exp, std::move(order)},
          Framework::Expand::UnsatCoreStrategy{exp, conf} {}

protected:
    using Strategy::getVerifier;

    // using Strategy::assertFormulaExplanation allows to handle those explanations as well
};
} // namespace xspace::expand::opensmt

#endif // XSPACE_EXPAND_OSMTUCORESTRATEGY_H
