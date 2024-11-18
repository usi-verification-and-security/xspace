#ifndef XAI_SMT_UCOREVERIFIER_H
#define XAI_SMT_UCOREVERIFIER_H

#include "Verifier.h"

#include <vector>

namespace xai::verifiers {

struct UnsatCore {
    std::vector<NodeIndex> equalities;
    std::vector<NodeIndex> lowerBounds;
    std::vector<NodeIndex> upperBounds;
};

class UnsatCoreVerifier : public Verifier {
public:
    virtual UnsatCore getUnsatCore() const = 0;
};
}

#endif //XAI_SMT_UCOREVERIFIER_H
