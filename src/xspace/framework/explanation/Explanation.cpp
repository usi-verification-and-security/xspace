#include "Explanation.h"

#include <algorithm>

namespace xspace {
void Explanation::swap(Explanation & rhs) {
    std::swap(frameworkPtr, rhs.frameworkPtr);
}
} // namespace xspace
