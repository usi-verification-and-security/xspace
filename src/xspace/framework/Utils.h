#ifndef XSPACE_FRAMEWORK_UTILS_H
#define XSPACE_FRAMEWORK_UTILS_H

#include "VarBound.h"
#include "Framework.h"

#include <xspace/common/Interval.h>
#include <xspace/common/Var.h>

#include <optional>

namespace xspace {
Interval varBoundToInterval(Framework const &, VarIdx, VarBound const &);
inline Interval optVarBoundToInterval(Framework const & framework, VarIdx idx,
                                      std::optional<VarBound> const & optVarBnd) {
    if (optVarBnd.has_value()) { return varBoundToInterval(framework, idx, *optVarBnd); }

    return framework.getDomainInterval(idx);
}
} // namespace xspace

#endif // XSPACE_FRAMEWORK_UTILS_H
