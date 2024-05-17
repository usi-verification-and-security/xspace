#ifndef XAI_SMT_BASICVERIX_H
#define XAI_SMT_BASICVERIX_H

#include <string>
#include "Verifier.h"

namespace xai::algo {
class BasicVerix {
public:
    struct Result {
        std::vector<NodeIndex> explanation;
    };

    explicit BasicVerix(std::string networkFile);

    void setVerifier(std::unique_ptr<Verifier> verifier);

    Result computeExplanation(std::vector<float> const & inputValues, float freedom_factor);

private:
    // TODO: Should it have NN already
    std::string networkFile;
    std::unique_ptr<Verifier> verifier;

};
} // namespace xai::algo

#endif //XAI_SMT_BASICVERIX_H
