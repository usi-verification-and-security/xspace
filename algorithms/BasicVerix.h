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

    struct GeneralizedExplanation {
        enum class BoundType {EQ, LB, UB};
        struct Constraint {
            NodeIndex inputIndex;
            BoundType boundType;
            float value;

            std::string opString() const {
                switch (boundType) {
                    case BoundType::EQ:
                        return "=";
                    case BoundType::LB:
                        return ">=";
                    case BoundType::UB:
                        return "<=";
            }
        }
        };
        std::vector<Constraint> constraints;


    };

    explicit BasicVerix(std::string networkFile);

    void setVerifier(std::unique_ptr<Verifier> verifier);

    Result computeExplanation(std::vector<float> const & inputValues, float freedom_factor,
                              std::vector<std::size_t> const & featureOrder);

    /**
     * Computes explanation not only as a subset of features, but also tries to relax the constraints on the relevant features
     */
    GeneralizedExplanation computeGeneralizedExplanation(std::vector<float> const & inputValues,
                                 std::vector<std::size_t> const & featureOrder, int threshold);

    // not compatible with `computeGeneralizedExplanation` yet
    Result computeOpenSMTExplanation(std::vector<float> const & inputValues, float freedom_factor,
                                     std::vector<std::size_t> const & featureOrder);

protected:
    void encodeClassificationConstraint(std::vector<float> const & output, NodeIndex label);

    Verifier::Answer check();

    std::unique_ptr<NNet> network;
    std::unique_ptr<Verifier> verifier;

    std::size_t checksCount;
};

} // namespace xai::algo

#endif //XAI_SMT_BASICVERIX_H
