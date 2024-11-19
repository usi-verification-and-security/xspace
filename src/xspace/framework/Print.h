#ifndef XSPACE_FRAMEWORK_PRINT_H
#define XSPACE_FRAMEWORK_PRINT_H

#include "Framework.h"

#include <xspace/common/Print.h>

#include <cassert>
#include <iosfwd>

namespace xspace {
class Framework::Print {
public:
    Print(Framework const &);

    bool ignoringStats() const { return ignoring(statsOsPtr); }
    bool ignoringBounds() const { return ignoring(boundsOsPtr); }
    bool ignoringFormulas() const { return ignoring(phiOsPtr); }

    std::ostream & stats() const {
        assert(statsOsPtr);
        return *statsOsPtr;
    }
    std::ostream & bounds() const {
        assert(boundsOsPtr);
        return *boundsOsPtr;
    }
    std::ostream & formulas() const {
        assert(phiOsPtr);
        return *phiOsPtr;
    }

protected:
    struct Absorb : std::ostream {
        std::ostream & operator<<(auto const &) { return *this; }
    };

    bool ignoring(std::ostream * osPtr) const {
        assert(osPtr);
        return osPtr == &absorb;
    }

    Framework const & framework;

    std::ostream * statsOsPtr{&absorb};
    std::ostream * boundsOsPtr{&absorb};
    std::ostream * phiOsPtr{&absorb};

    Absorb absorb{};
};
} // namespace xspace

#endif // XSPACE_FRAMEWORK_PRINT_H
