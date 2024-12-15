#include "Explanation.h"

namespace xspace {
void Explanation::swap(Explanation & rhs) {
    std::swap(frameworkPtr, rhs.frameworkPtr);
}
} // namespace xspace
