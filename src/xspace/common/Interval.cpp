#include "Interval.h"

#include <ostream>

namespace xspace {
void Interval::print(std::ostream & os) const {
    os << '[' << getLower() << ',' << getUpper() << ']';
}
} // namespace xspace
