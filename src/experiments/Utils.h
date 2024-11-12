#include <string>
#include <regex>
#include <cassert>

#include <common/StringConv.h>

namespace xai::experiments {
[[nodiscard]]
inline std::string fixOpenSMTString(std::string str) {
    static const std::regex re_minus(R"(-([0-9]+[0-9/]*))");
    str = std::regex_replace(str.c_str(), re_minus, "(- $1)");

    static const std::regex re_div(R"(([0-9]+)/([0-9]+))");
    str = std::regex_replace(str.c_str(), re_div, "(/ $1 $2)");

    return str;
}

inline std::string floatToRationalString(float value) {
    auto s = std::to_string(value);
    char* rationalString;
    opensmt::stringToRational(rationalString, s.c_str());
    std::string str{rationalString};
    free(rationalString);

    return fixOpenSMTString(std::move(str));
}
}
