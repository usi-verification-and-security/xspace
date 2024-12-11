#include "Print.h"

//+ ideally get rid of these
#include <common/FastRational.h>
#include <common/StringConv.h>

#include <string>

namespace xspace {
namespace {
    //+ to be consistent with BasicVerix, but ideally should not depend on OpenSMT
    opensmt::FastRational floatToFastRational(Float value) {
        //! approximation
        auto s = std::to_string(value);
        char * rationalString;
        opensmt::stringToRational(rationalString, s.c_str());
        auto res = opensmt::FastRational(rationalString);
        free(rationalString);
        return res;
    }
} // namespace

void printSmtLib2AsRational(std::ostream & os, Float val) {
    auto const rat = floatToFastRational(val);
    os << rat;
}
} // namespace xspace
