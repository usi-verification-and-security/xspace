#ifndef XSPACE_EXPAND_OSMTITPSTRATEGY_H
#define XSPACE_EXPAND_OSMTITPSTRATEGY_H

#include "Strategy.h"

namespace xspace::expand::opensmt {
class InterpolationStrategy : public Strategy {
public:
    enum class BoolInterpolationAlg { weak, strong };
    enum class ArithInterpolationAlg { weak, weaker, strong, stronger };

    struct Config {
        BoolInterpolationAlg boolInterpolationAlg{BoolInterpolationAlg::strong};
        ArithInterpolationAlg arithInterpolationAlg{ArithInterpolationAlg::weak};
    };

    using Strategy::Strategy;
    InterpolationStrategy(Framework::Expand & exp, Config const & conf, Framework::Expand::VarOrdering order = {})
        : Framework::Expand::Strategy{exp, std::move(order)},
          config{conf} {}

    static char const * name() { return "itp"; }

    bool isAbductiveOnly() const override { return false; }

protected:
    void executeInit(std::unique_ptr<Explanation> &) override;
    void executeBody(std::unique_ptr<Explanation> &) override;

    Config config{};
};
} // namespace xspace::expand::opensmt

#endif // XSPACE_EXPAND_OSMTITPSTRATEGY_H
