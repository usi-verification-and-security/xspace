#ifndef XSPACE_EXPAND_UCORESTRATEGY_H
#define XSPACE_EXPAND_UCORESTRATEGY_H

#include "Strategy.h"

namespace xai::verifiers {
class UnsatCoreVerifier;
}

namespace xspace {
class Framework::Expand::UnsatCoreStrategy : virtual public Strategy {
public:
    struct Config {
        bool splitEq = false;
    };

    using Strategy::Strategy;
    UnsatCoreStrategy(Expand & exp, Config const & conf, VarOrdering order = {})
        : Strategy{exp, std::move(order)},
          config{conf} {}

    static char const * name() { return "ucore"; }

    bool isAbductiveOnly() const override { return not config.splitEq; }

protected:
    xai::verifiers::UnsatCoreVerifier & getVerifier();

    bool storeNamedTerms() const override { return true; }

    void executeBody(std::unique_ptr<Explanation> &) override;

    Config config{};
};
} // namespace xspace

#endif // XSPACE_EXPAND_UCORESTRATEGY_H
