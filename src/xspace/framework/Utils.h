#ifndef XSPACE_FRAMEWORK_UTILS_H
#define XSPACE_FRAMEWORK_UTILS_H

#include "Framework.h"
#include "explanation/VarBound.h"

#include <xspace/common/Interval.h>
#include <xspace/common/Var.h>

#include <memory>

namespace xspace {
std::unique_ptr<VarBound> intervalToOptVarBound(Framework const &, VarIdx, Interval const &);
} // namespace xspace

#endif // XSPACE_FRAMEWORK_UTILS_H
