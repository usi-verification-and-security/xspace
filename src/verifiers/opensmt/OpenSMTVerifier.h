#ifndef XAI_SMT_OPENSMTVERIFIER_H
#define XAI_SMT_OPENSMTVERIFIER_H

#include <verifiers/Verifier.h>

namespace opensmt {
class MainSolver;
struct PTRef;
}

namespace xai::verifiers {

class OpenSMTVerifier : public Verifier{
public:
    OpenSMTVerifier();
    ~OpenSMTVerifier();

    void loadModel(nn::NNet const & network) override;

    void addUpperBound(LayerIndex layer, NodeIndex var, float value, bool namedTerm = false) override;

    void addLowerBound(LayerIndex layer, NodeIndex var, float value, bool namedTerm = false) override;

    void addEquality(LayerIndex layer, NodeIndex var, float value, bool namedTerm = false) override;

    void addClassificationConstraint(NodeIndex node, float threshold) override;

    void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) override;

    void push() override;
    void pop() override;

    Answer check() override;

    opensmt::MainSolver & getSolver();

    bool containsInputLowerBound(opensmt::PTRef const & term) const;
    bool containsInputUpperBound(opensmt::PTRef const & term) const;
    bool containsInputEquality(opensmt::PTRef const & term) const;
    NodeIndex nodeIndexOfInputLowerBound(opensmt::PTRef const & term) const;
    NodeIndex nodeIndexOfInputUpperBound(opensmt::PTRef const & term) const;
    NodeIndex nodeIndexOfInputEquality(opensmt::PTRef const & term) const;

private:
    class OpenSMTImpl;
    OpenSMTImpl * pimpl;
};

} // namespace xai::verifiers

#endif //XAI_SMT_OPENSMTVERIFIER_H
