#include "Print.h"

#include "Config.h"

#include <iostream>

namespace xspace {
Framework::Print::Print(Framework const & fw) : framework{fw}, phiOsPtr{&std::cerr} {
    auto const & conf = framework.getConfig();
    bool const verbose = conf.isVerbose();

    if (not verbose) { return; }

    statsOsPtr = &std::cout;
}
} // namespace xspace
