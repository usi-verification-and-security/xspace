#ifndef XSPACE_PARTIALEXPLANATION_H
#define XSPACE_PARTIALEXPLANATION_H

#include "../Framework.h"

#include <xspace/common/Var.h>

namespace xspace {
class PartialExplanation {
public:
    explicit PartialExplanation(Framework const & fw) : frameworkPtr{&fw} {}
    virtual ~PartialExplanation() = default;
    PartialExplanation(PartialExplanation const &) = default;
    PartialExplanation(PartialExplanation &&) = default;
    PartialExplanation & operator=(PartialExplanation const &) = default;
    PartialExplanation & operator=(PartialExplanation &&) = default;

    virtual std::size_t varSize() const;

    virtual bool contains(VarIdx) const = 0;

    void swap(PartialExplanation &);

    virtual void print(std::ostream & os) const { printSmtLib2(os); }
    virtual void printSmtLib2(std::ostream &) const = 0;

protected:
    Framework::Expand const & getExpand() const { return frameworkPtr->getExpand(); }

    Framework const * frameworkPtr;
};
} // namespace xspace

#endif // XSPACE_PARTIALEXPLANATION_H
