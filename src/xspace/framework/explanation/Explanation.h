#ifndef XSPACE_EXPLANATION_H
#define XSPACE_EXPLANATION_H

#include <xspace/common/Core.h>
#include <xspace/common/Var.h>

namespace xspace {
class Framework;

class Explanation {
public:
    Explanation(Framework const & fw) : frameworkPtr{&fw} {}

    virtual std::size_t varSize() const = 0;

    virtual bool contains(VarIdx) const = 0;

    virtual void clear() {}

    void swap(Explanation &);

    std::size_t getFixedCount() const { return computeFixedCount(); }

    virtual Float getRelativeVolume() const = 0;
    virtual Float getRelativeVolumeSkipFixed() const = 0;

    virtual void print(std::ostream & os) const { printSmtLib2(os); }
    virtual void printSmtLib2(std::ostream &) const = 0;

protected:
    virtual std::size_t computeFixedCount() const = 0;

    Framework const * frameworkPtr;
};
} // namespace xspace

#endif // XSPACE_EXPLANATION_H
