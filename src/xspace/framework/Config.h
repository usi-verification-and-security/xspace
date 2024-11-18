#ifndef XSPACE_CONFIG_H
#define XSPACE_CONFIG_H

#include "Framework.h"

namespace xspace {
class Framework::Config {
public:
    using Verbosity = short;

    void setVerbosity(Verbosity verb) { verbosity = verb; }
    void setVerbose() { setVerbosity(1); }

    Verbosity getVerbosity() const { return verbosity; }
    bool isVerbose() const { return getVerbosity() > 0; }

protected:
    Verbosity verbosity = 0;
};
} // namespace xspace

#endif // XSPACE_CONFIG_H
