#ifndef XSPACE_STRING_H
#define XSPACE_STRING_H

#include "Print.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace xspace {
inline std::string toString(printable auto const & arg) {
    std::ostringstream oss;
    arg.print(oss);
    return std::move(oss).str();
}

inline std::string_view toStringView(printable auto const & arg) {
    std::ostringstream oss;
    arg.print(oss);
    return std::move(oss).view();
}

inline std::string toLower(std::string_view sw) {
    std::string str{sw};
    std::ranges::transform(str, str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
}
inline std::string toUpper(std::string_view sw) {
    std::string str{sw};
    std::ranges::transform(str, str.begin(), [](unsigned char c) { return std::toupper(c); });
    return str;
}
} // namespace xspace

#endif // XSPACE_STRING_H
