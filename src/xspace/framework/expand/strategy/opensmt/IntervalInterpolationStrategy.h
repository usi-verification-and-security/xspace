#ifndef XSPACE_EXPAND_OSMTINTERVALITPSTRATEGY_H
#define XSPACE_EXPAND_OSMTINTERVALITPSTRATEGY_H

#include "InterpolationStrategy.h"

#include <xspace/framework/explanation/opensmt/FormulaExplanation.h>

#include <xspace/common/Bound.h>

namespace xspace::expand::opensmt {
class IntervalInterpolationStrategy : public InterpolationStrategy {
public:
    using InterpolationStrategy::InterpolationStrategy;
    IntervalInterpolationStrategy(Framework::Expand & exp, Config const & conf, Framework::Expand::VarOrdering order = {})
        : Framework::Expand::Strategy{exp, std::move(order)},
          InterpolationStrategy{exp, conf} {}

    static char const * name() { return "itp-interval"; }

protected:
    void executeBody(std::unique_ptr<Explanation> &) override;

    // std::pair<VarIdx, Bound> formulaToBound(Formula const &) const;
    std::pair<VarIdx, Bound> formulaToBound(Formula const &);
};
} // namespace xspace::expand::opensmt

#endif // XSPACE_EXPAND_OSMTINTERVALITPSTRATEGY_H
