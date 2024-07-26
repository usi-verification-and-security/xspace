#ifndef XAI_SMT_ITPOPENSMTVERIFIER_H
#define XAI_SMT_ITPOPENSMTVERIFIER_H

#include "Verifier.h"

namespace xai::verifiers {

class InterpolatingOpenSMTVerifier : public InterpolatingVerifier {
public:
    InterpolatingOpenSMTVerifier();
    ~InterpolatingOpenSMTVerifier();

    void loadModel(NNet const & network) override;

    void addUpperBound(LayerIndex layer, NodeIndex var, float value) override;

    void addLowerBound(LayerIndex layer, NodeIndex var, float value) override;

    void addClassificationConstraint(NodeIndex node, float threshold) override;

    void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) override;

    Answer check() override;

    void pushCheckpoint() override;
    void popCheckpoint() override;

    Explanation explain() override;


private:
    class InterpolatingOpenSMTImpl;
    InterpolatingOpenSMTImpl * pimpl;
};

} // namespace xai::verifiers

#endif //XAI_SMT_ITPOPENSMTVERIFIER_H
