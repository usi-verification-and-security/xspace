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
    struct Config {};

    using Strategy::Strategy;
    OpenSMTInterpolationStrategy(Expand & exp, Config const & conf, VarOrdering order = {})
        : Strategy{exp, std::move(order)},
          config{conf} {}

    bool isAbductiveOnly() const override { return false; }

protected:
    xai::verifiers::OpenSMTVerifier & getVerifier();

    void executeBody(std::unique_ptr<Explanation> &) override;

    bool assertExplanationImpl(Explanation const &, AssertExplanationConf const &) override;

    void assertFormulaExplanation(opensmt::FormulaExplanation const &);
    void assertFormulaExplanation(opensmt::FormulaExplanation const &, AssertExplanationConf const &);

    Config config{};
};
} // namespace xspace

#endif // XSPACE_EXPAND_OSMTITPSTRATEGY_H
