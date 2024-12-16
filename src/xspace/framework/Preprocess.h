#ifndef XSPACE_PREPROCESS_H
#define XSPACE_PREPROCESS_H

#include "Framework.h"

#include <xspace/nn/Dataset.h>

namespace xspace {
class Framework::Preprocess {
public:
    Preprocess(Framework &);

    Explanations operator()(Dataset &);

    static bool isBinaryClassification(Dataset::Output::Values const &);

    static Dataset::Classification::Label computeClassificationLabel(Dataset::Output::Values const &);
    static Dataset::Classification::Label computeBinaryClassificationLabel(Dataset::Output::Values const &);
    static Dataset::Classification::Label computeNonBinaryClassificationLabel(Dataset::Output::Values const &);

protected:
    Dataset::Output computeOutput(Dataset::Sample const &) const;

    Framework & framework;
};
} // namespace xspace

#endif // XSPACE_PREPROCESS_H
