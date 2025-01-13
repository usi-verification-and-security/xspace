#ifndef XSPACE_VAR_H
#define XSPACE_VAR_H

#include <cassert>
#include <string>

using namespace std::string_literals;

namespace xspace {
using VarIdx = std::size_t;
using VarName = std::string;

constexpr VarIdx invalidVarIdx = -1;

inline VarName makeVarName(VarIdx idx) {
    return "x"s + std::to_string(idx + 1);
}

inline VarIdx getVarIdx(VarName const & varName) {
    auto idxStr = varName.substr(1);
    VarIdx idx = std::stoi(std::move(idxStr));
    assert(idx >= 1);
    return idx - 1;
}
} // namespace xspace

#endif // XSPACE_VAR_H
