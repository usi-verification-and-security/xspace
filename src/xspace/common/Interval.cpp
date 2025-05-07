#include "Interval.h"

#include <ostream>

namespace xspace {
void Interval::intersect(Interval && rhs) {
    assert(isValid());
    assert(rhs.isValid());

    setLower(std::max(getLower(), rhs.getLower()));
    setUpper(std::min(getUpper(), rhs.getUpper()));
}

void Interval::print(std::ostream & os) const {
    os << '[' << getLower() << ',' << getUpper() << ']';
}
} // namespace xspace
