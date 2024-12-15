#ifndef XSPACE_OSMTEXPLANATION_H
#define XSPACE_OSMTEXPLANATION_H

#include "../Explanation.h"

#include <memory>

namespace opensmt {
struct PTRef;
} // namespace opensmt

namespace xai::verifiers {
class OpenSMTVerifier;
}

namespace xspace::opensmt {
using Formula = ::opensmt::PTRef;

class FormulaExplanation : public Explanation {
public:
    using Explanation::Explanation;
    FormulaExplanation(Framework const &, Formula const &);

    Formula const & getFormula() const { return *formulaPtr; }

    std::size_t varSize() const override;

    bool contains(VarIdx) const override;

    void clear() override;

    void swap(FormulaExplanation &);

    Float getRelativeVolume() const override;
    Float getRelativeVolumeSkipFixed() const override;

    void printSmtLib2(std::ostream &) const override;

protected:
    xai::verifiers::OpenSMTVerifier const & getVerifier() const;

    void resetFormula() { formulaPtr.reset(); }

    std::size_t computeFixedCount() const override;

    std::unique_ptr<Formula> formulaPtr{};
};
} // namespace xspace::opensmt

#endif // XSPACE_OSMTEXPLANATION_H
