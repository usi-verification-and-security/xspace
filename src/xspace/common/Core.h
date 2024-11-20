#ifndef XSPACE_CORE_H
#define XSPACE_CORE_H

#include <utility>

#define FORWARD(arg) std::forward<decltype(arg)>(arg)

namespace xspace {
using Float = float;
} // namespace xspace

#endif // XSPACE_CORE_H
