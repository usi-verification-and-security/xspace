#ifndef XSPACE_COMMON_UTILS_H
#define XSPACE_COMMON_UTILS_H

#include "Macro.h"

#include <concepts>
#include <memory>

namespace xspace {
template<typename Derived, typename Base>
    requires std::derived_from<Derived, Base>
void assignNew(std::unique_ptr<Base> & ptr, auto &&... args) {
    if (auto derivedPtr = dynamic_cast<Derived *>(ptr.get())) {
        *derivedPtr = Derived{FORWARD(args)...};
    } else {
        ptr = std::make_unique<Derived>(FORWARD(args)...);
    }
}
} // namespace xspace

#endif // XSPACE_COMMON_UTILS_H
