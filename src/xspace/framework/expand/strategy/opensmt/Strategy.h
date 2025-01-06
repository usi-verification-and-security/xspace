#ifndef XSPACE_EXPAND_OSMTSTRATEGY_H
#define XSPACE_EXPAND_OSMTSTRATEGY_H

#include "../Strategy.h"

namespace xai::verifiers {
class OpenSMTVerifier;
}

namespace xspace::opensmt {
class FormulaExplanation;
}

namespace xspace::expand::opensmt {
using namespace xspace::opensmt;

//+ make templated with Formula
class Strategy : virtual public Framework::Expand::Strategy {
protected:
    xai::verifiers::OpenSMTVerifier & getVerifier();

    bool assertExplanationImpl(PartialExplanation const &, AssertExplanationConf const &) override;

    void assertFormulaExplanation(FormulaExplanation const &);
    void assertFormulaExplanation(FormulaExplanation const &, AssertExplanationConf const &);
};
} // namespace xspace::expand::opensmt

#endif // XSPACE_EXPAND_OSMTSTRATEGY_H
