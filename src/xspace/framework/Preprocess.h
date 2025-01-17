#ifndef XSPACE_PREPROCESS_H
#define XSPACE_PREPROCESS_H

#include "Framework.h"

#include <xspace/nn/Dataset.h>

namespace xspace {
class Framework::Preprocess {
public:
    Preprocess(Framework &, Dataset &);

    Explanations makeExplanationsFromSamples() const;

    static bool isBinaryClassification(Dataset::Output::Values const &);

    static Dataset::Classification::Label computeClassificationLabel(Dataset::Output::Values const &);
    static Dataset::Classification::Label computeBinaryClassificationLabel(Dataset::Output::Values const &);
    static Dataset::Classification::Label computeNonBinaryClassificationLabel(Dataset::Output::Values const &);

protected:
    void initDataset();

    Dataset::Output computeOutput(Dataset::Sample const &) const;

    Framework & framework;

    Dataset & dataset;
};
} // namespace xspace

#endif // XSPACE_PREPROCESS_H
