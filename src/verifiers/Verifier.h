#ifndef XAI_SMT_VERIFIER_H
#define XAI_SMT_VERIFIER_H

#include <nn/NNet.h>

#include <string>
#include <vector>

namespace xai::verifiers {

using NodeIndex = std::size_t;
using LayerIndex = std::size_t;

class Verifier {
public:
    enum class Answer { SAT, UNSAT, UNKNOWN, ERROR };

    Verifier() = default;
    virtual ~Verifier() = default;
    Verifier(Verifier const &) = delete;
    Verifier & operator=(Verifier const &) = delete;
    Verifier(Verifier &&) = default;
    Verifier & operator=(Verifier &&) = default;

    virtual void loadModel(nn::NNet const &) = 0;

    virtual void addUpperBound(LayerIndex layer, NodeIndex var, float value, bool explanationTerm = false) = 0;
    virtual void addLowerBound(LayerIndex layer, NodeIndex var, float value, bool explanationTerm = false) = 0;
    virtual void addEquality(LayerIndex layer, NodeIndex var, float value, bool explanationTerm = false) {
        addUpperBound(layer, var, value, explanationTerm);
        addLowerBound(layer, var, value, explanationTerm);
    }
    virtual void addClassificationConstraint(NodeIndex node, float threshold) = 0;

    virtual void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) = 0;

    virtual void init() {
        initImpl();
        reset();
    }

    virtual void push() = 0;
    virtual void pop() = 0;

    virtual Answer check() {
        ++checksCount;
        return checkImpl();
    }

    std::size_t getChecksCount() const { return checksCount; }

    virtual void resetSampleQuery() {}
    virtual void resetSample() {
        resetSampleQuery();
        checksCount = 0;
    }
    virtual void reset() { resetSample(); }

protected:
    virtual void initImpl() {}

    virtual Answer checkImpl() = 0;

    std::size_t checksCount{};
};
} // namespace xai::verifiers

#endif // XAI_SMT_VERIFIER_H
