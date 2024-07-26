#ifndef XAI_SMT_ITPBASEDEXPLANATIONS_H
#define XAI_SMT_ITPBASEDEXPLANATIONS_H

#include "Verifier.h"

namespace xai::algo {

class ITPBasedExplanations {
public:
    struct Result {
        std::vector<NodeIndex> explanation;
    };

    explicit ITPBasedExplanations(std::string networkFile);

    void setVerifier(std::unique_ptr<InterpolatingVerifier> verifier);

    Result computeExplanation(std::vector<float> const &inputValues);

private:
    void encodeClassificationConstraint(std::vector<float> const &output, NodeIndex label);

    std::unique_ptr<NNet> network;
    std::unique_ptr<InterpolatingVerifier> verifier;

};

} // namespace xai


#endif //XAI_SMT_ITPBASEDEXPLANATIONS_H
