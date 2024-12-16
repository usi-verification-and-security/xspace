#ifndef XSPACE_EXPAND_OSMTITPSTRATEGY_H
#define XSPACE_EXPAND_OSMTITPSTRATEGY_H

#include "Strategy.h"

namespace xai::verifiers {
class OpenSMTVerifier;
}

namespace xspace::opensmt {
class FormulaExplanation;
}

namespace xspace {
class Framework::Expand::OpenSMTInterpolationStrategy : public Strategy {
public:
    enum class BoolInterpolationAlg { weak, strong };
    enum class ArithInterpolationAlg { weak, weaker, strong, stronger };

    struct Config {
        BoolInterpolationAlg boolInterpolationAlg{BoolInterpolationAlg::strong};
        ArithInterpolationAlg arithInterpolationAlg{ArithInterpolationAlg::weak};
    };

    using Strategy::Strategy;
    OpenSMTInterpolationStrategy(Expand & exp, Config const & conf, VarOrdering order = {})
        : Strategy{exp, std::move(order)},
          config{conf} {}

    bool isAbductiveOnly() const override { return false; }

protected:
    xai::verifiers::OpenSMTVerifier & getVerifier();

    void executeInit(std::unique_ptr<Explanation> &) override;
    void executeBody(std::unique_ptr<Explanation> &) override;

    bool assertExplanationImpl(Explanation const &, AssertExplanationConf const &) override;

    void assertFormulaExplanation(opensmt::FormulaExplanation const &);
    void assertFormulaExplanation(opensmt::FormulaExplanation const &, AssertExplanationConf const &);

    Config config{};
};
} // namespace xspace

#endif // XSPACE_EXPAND_OSMTITPSTRATEGY_H
