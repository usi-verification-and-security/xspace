#ifndef XAI_SMT_VERIFIER_H
#define XAI_SMT_VERIFIER_H

#include "NNet.h"

#include <string>
#include <vector>

namespace xai {

using NodeIndex = std::size_t;
using LayerIndex = std::size_t;

class Verifier {
public:
    enum class Answer {SAT, UNSAT, UNKNOWN, ERROR};

    virtual void loadModel(NNet const &) = 0;

    virtual void addUpperBound(LayerIndex layer, NodeIndex var, float value) = 0;
    virtual void addLowerBound(LayerIndex layer, NodeIndex var, float value) = 0;
    virtual void addClassificationConstraint(NodeIndex node, float threshold) = 0;

    virtual void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) = 0;

    virtual Answer check() = 0;

    virtual void clearAdditionalConstraints() = 0;

    virtual ~Verifier() = default;
};

class InterpolatingVerifier {
public:
    enum class Answer {SAT, UNSAT, UNKNOWN, ERROR};
    using Explanation = std::vector<std::pair<LayerIndex, NodeIndex>>;

    virtual void loadModel(NNet const &) = 0;

    virtual void addUpperBound(LayerIndex layer, NodeIndex var, float value) = 0;
    virtual void addLowerBound(LayerIndex layer, NodeIndex var, float value) = 0;
    virtual void addClassificationConstraint(NodeIndex node, float threshold) = 0;

    virtual void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) = 0;

    virtual Answer check() = 0;

    virtual void pushCheckpoint() = 0;
    virtual void popCheckpoint() = 0;

    virtual Explanation explain() = 0;

    virtual ~InterpolatingVerifier() = default;
};

}

#endif //XAI_SMT_VERIFIER_H
