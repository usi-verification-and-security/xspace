#ifndef XSPACE_CORE_H
#define XSPACE_CORE_H

#include <iosfwd>
#include <sstream>
#include <string>
#include <string_view>

namespace xspace {
template<typename T>
concept printable = requires(T const & t, std::ostream & os) { t.print(os); };

using Float = float;

std::ostream & operator<<(std::ostream & os, printable auto const & rhs) {
    rhs.print(os);
    return os;
}

std::string toString(printable auto const & arg) {
    std::ostringstream oss;
    arg.print(oss);
    return std::move(oss).str();
}

std::string_view toStringView(printable auto const & arg) {
    std::ostringstream oss;
    arg.print(oss);
    return std::move(oss).view();
}
} // namespace xspace

#endif // XSPACE_CORE_H
