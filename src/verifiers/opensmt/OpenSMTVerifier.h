#ifndef XAI_SMT_OPENSMTVERIFIER_H
#define XAI_SMT_OPENSMTVERIFIER_H

#include <verifiers/UnsatCoreVerifier.h>

namespace opensmt {
class MainSolver;
struct PTRef;
}

namespace xai::verifiers {

class OpenSMTVerifier : public UnsatCoreVerifier {
public:
    OpenSMTVerifier();
    ~OpenSMTVerifier();

    void loadModel(nn::NNet const & network) override;

    void addUpperBound(LayerIndex layer, NodeIndex var, float value, bool explanationTerm = false) override;

    void addLowerBound(LayerIndex layer, NodeIndex var, float value, bool explanationTerm = false) override;

    void addEquality(LayerIndex layer, NodeIndex var, float value, bool explanationTerm = false) override;

    void addClassificationConstraint(NodeIndex node, float threshold) override;

    void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) override;

    void push() override;
    void pop() override;

    void resetSample() override;
    void reset() override;

    UnsatCore getUnsatCore() const override;

    opensmt::MainSolver const & getSolver() const;
    opensmt::MainSolver & getSolver();

protected:
    void initImpl() override;

    Answer checkImpl() override;

private:
    class OpenSMTImpl;
    OpenSMTImpl * pimpl;
};

} // namespace xai::verifiers

#endif //XAI_SMT_OPENSMTVERIFIER_H
