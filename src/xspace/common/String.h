#ifndef XSPACE_STRING_H
#define XSPACE_STRING_H

#include <algorithm>
#include <string>
#include <string_view>

namespace xspace {
inline std::string tolower(std::string_view sw) {
    std::string str{sw};
    std::ranges::transform(str, str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
}
inline std::string toupper(std::string_view sw) {
    std::string str{sw};
    std::ranges::transform(str, str.begin(), [](unsigned char c) { return std::toupper(c); });
    return str;
}
} // namespace xspace

#endif // XSPACE_STRING_H
