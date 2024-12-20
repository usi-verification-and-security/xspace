#ifndef XSPACE_EXPAND_ABDUCTIVESTRATEGY_H
#define XSPACE_EXPAND_ABDUCTIVESTRATEGY_H

#include "Strategy.h"

namespace xspace {
class Framework::Expand::AbductiveStrategy : public Strategy {
public:
    using Strategy::Strategy;

    static char const * name() { return "abductive"; }

protected:
    void executeBody(std::unique_ptr<Explanation> &) override;
};
} // namespace xspace

#endif // XSPACE_EXPAND_ABDUCTIVESTRATEGY_H
