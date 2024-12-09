#ifndef XSPACE_FRAMEWORK_UTILS_H
#define XSPACE_FRAMEWORK_UTILS_H

#include "Framework.h"

namespace xspace {
Interval varBoundToInterval(Framework const &, VarIdx, VarBound const &);
inline Interval optVarBoundToInterval(Framework const & framework, VarIdx idx,
                                      std::optional<VarBound> const & optVarBnd) {
    if (optVarBnd.has_value()) { return varBoundToInterval(framework, idx, *optVarBnd); }

    return framework.getDomainInterval(idx);
}
} // namespace xspace

#endif // XSPACE_FRAMEWORK_UTILS_H
