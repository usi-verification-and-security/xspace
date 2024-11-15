#ifndef XAI_SMT_MARABOUVERIFIER_H
#define XAI_SMT_MARABOUVERIFIER_H

#include <verifiers/Verifier.h>


namespace xai::verifiers {

class MarabouVerifier : public Verifier {
public:
    MarabouVerifier();
    ~MarabouVerifier();

    void loadModel(nn::NNet const & network) override;

    void addUpperBound(LayerIndex layer, NodeIndex var, float value, bool namedTerm = false) override;

    void addLowerBound(LayerIndex layer, NodeIndex var, float value, bool namedTerm = false) override;

    void addClassificationConstraint(NodeIndex node, float threshold) override;

    void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) override;

    void push() override;
    void pop() override;

    Answer check() override;

private:
    class MarabouImpl;
    MarabouImpl * pimpl;
};
}



#endif //XAI_SMT_MARABOUVERIFIER_H
