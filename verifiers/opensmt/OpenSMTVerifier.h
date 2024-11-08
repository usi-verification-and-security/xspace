#ifndef XAI_SMT_OPENSMTVERIFIER_H
#define XAI_SMT_OPENSMTVERIFIER_H

#include "Verifier.h"

namespace xai::verifiers {

class OpenSMTVerifier : public Verifier{
public:
    OpenSMTVerifier();
    ~OpenSMTVerifier();

    void loadModel(NNet const & network) override;

    void addUpperBound(LayerIndex layer, NodeIndex var, float value) override;

    void addLowerBound(LayerIndex layer, NodeIndex var, float value) override;

    void addEquality(LayerIndex layer, NodeIndex var, float value) override;

    void addClassificationConstraint(NodeIndex node, float threshold) override;

    void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) override;

    Answer check() override;

    void clearAdditionalConstraints() override;

private:
    class OpenSMTImpl;
    OpenSMTImpl * pimpl;
};

} // namespace xai::verifiers

#endif //XAI_SMT_OPENSMTVERIFIER_H
