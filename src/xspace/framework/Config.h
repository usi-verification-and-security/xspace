#ifndef XSPACE_CONFIG_H
#define XSPACE_CONFIG_H

#include "Framework.h"

namespace xspace {
class Framework::Config {
public:
    using Verbosity = short;

    void setVerbosity(Verbosity verb) { verbosity = verb; }
    void beVerbose() { setVerbosity(1); }

    void reverseVarOrdering() { reverseVarOrder = true; }

    Verbosity getVerbosity() const { return verbosity; }
    bool isVerbose() const { return getVerbosity() > 0; }

    bool isReverseVarOrdering() const { return reverseVarOrder; }

protected:
    Verbosity verbosity{};

    bool reverseVarOrder{};
};
} // namespace xspace

#endif // XSPACE_CONFIG_H
