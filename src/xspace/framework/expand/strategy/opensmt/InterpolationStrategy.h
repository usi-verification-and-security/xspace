#ifndef XSPACE_EXPAND_OSMTITPSTRATEGY_H
#define XSPACE_EXPAND_OSMTITPSTRATEGY_H

#include "../Strategy.h"

namespace xai::verifiers {
class OpenSMTVerifier;
}

namespace xspace::opensmt {
class FormulaExplanation;
}

namespace xspace::expand::opensmt {
using namespace xspace::opensmt;

//+ make templated with Formula
class InterpolationStrategy : public Framework::Expand::Strategy {
public:
    enum class BoolInterpolationAlg { weak, strong };
    enum class ArithInterpolationAlg { weak, weaker, strong, stronger };

    struct Config {
        BoolInterpolationAlg boolInterpolationAlg{BoolInterpolationAlg::strong};
        ArithInterpolationAlg arithInterpolationAlg{ArithInterpolationAlg::weak};
    };

    using Strategy::Strategy;
    InterpolationStrategy(Framework::Expand & exp, Config const & conf, Framework::Expand::VarOrdering order = {})
        : Strategy{exp, std::move(order)},
          config{conf} {}

    static char const * name() { return "itp"; }

    bool isAbductiveOnly() const override { return false; }

protected:
    xai::verifiers::OpenSMTVerifier & getVerifier();

    void executeInit(std::unique_ptr<Explanation> &) override;
    void executeBody(std::unique_ptr<Explanation> &) override;

    bool assertExplanationImpl(PartialExplanation const &, AssertExplanationConf const &) override;

    void assertFormulaExplanation(FormulaExplanation const &);
    void assertFormulaExplanation(FormulaExplanation const &, AssertExplanationConf const &);

    Config config{};
};
} // namespace xspace::expand::opensmt

#endif // XSPACE_EXPAND_OSMTITPSTRATEGY_H
