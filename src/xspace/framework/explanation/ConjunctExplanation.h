#ifndef XSPACE_CONJEXPLANATION_H
#define XSPACE_CONJEXPLANATION_H

#include "Explanation.h"

#include <memory>
#include <vector>

namespace xspace {
class ConjunctExplanation : public Explanation {
public:
    struct PrintConfig {
        char delim = ' ';
    };

    using Explanation::Explanation;
    explicit ConjunctExplanation(Framework const & fw, std::size_t size_) : Explanation{fw}, conjunction(size_) {}

    // May contain null pointers representing true values
    auto begin() const { return conjunction.cbegin(); }
    auto end() const { return conjunction.cend(); }

    // Including null pointers
    std::size_t size() const { return conjunction.size(); }

    // May return null pointer ~ true value
    PartialExplanation const * tryGetExplanation(std::size_t idx) const { return getExplanationPtr(idx).get(); }
    PartialExplanation * tryGetExplanation(std::size_t idx) { return getExplanationPtr(idx).get(); }

    bool contains(VarIdx) const override;

    void clear() override;

    void swap(ConjunctExplanation &);

    virtual void insertExplanation(std::unique_ptr<PartialExplanation>);
    virtual void setExplanation(std::size_t idx, std::unique_ptr<PartialExplanation>);
    // Effectively sets the partial explanation to true value
    virtual bool eraseExplanation(std::size_t idx);

    void printSmtLib2(std::ostream &) const override;
    void printSmtLib2(std::ostream &, PrintConfig const &) const;

protected:
    std::unique_ptr<PartialExplanation> const & getExplanationPtr(std::size_t idx) const;
    std::unique_ptr<PartialExplanation> & getExplanationPtr(std::size_t idx);

    //+ also store indices to a set and iterate using it if the vector is already too sparse
    //+ see `assert-interval-maybe-using-map` git tag
    std::vector<std::unique_ptr<PartialExplanation>> conjunction{};
};
} // namespace xspace

#endif // XSPACE_CONJEXPLANATION_H
