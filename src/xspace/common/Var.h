#ifndef XSPACE_VAR_H
#define XSPACE_VAR_H

#include <string>

using namespace std::string_literals;

namespace xspace {
using VarIdx = std::size_t;
using VarName = std::string;

inline VarName makeVarName(VarIdx idx) {
    return "x"s + std::to_string(idx + 1);
}
} // namespace xspace

#endif // XSPACE_VAR_H
