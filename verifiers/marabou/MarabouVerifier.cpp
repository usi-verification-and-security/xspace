#include "MarabouVerifier.h"

#include "Engine.h"
#include "InputQuery.h"

#include <cassert>
#include <numeric>

namespace xai::verifiers {

namespace {
enum class VariableType {BACKWARD, FORWARD};
using VarIndex = std::size_t;

class QueryIncrementalWrapper {
public:
    static std::unique_ptr<QueryIncrementalWrapper> fromNNet(NNet const & net);

    std::unique_ptr<InputQuery> buildQuery() const;

    void clearAdditionalConstraints();

    void setLowerBound(LayerIndex layerNum, NodeIndex nodeIndex, float);
    void setUpperBound(LayerIndex layerNum, NodeIndex nodeIndex, float);

private:
    std::size_t registerNewVariable();
    VarIndex getVarIndex(LayerIndex layerNum, NodeIndex nodeIndex, VariableType variableType) const;

    void markInputVariable(VarIndex);
    void markOutputVariable(VarIndex);

    void addStructuralEquation(Equation eq) { structuralEquations.push_back(std::move(eq)); }
    void setHardInputLowerBound(VarIndex, float);
    void setHardInputUpperBound(VarIndex, float);

    std::size_t numVars{0};
    std::vector<VarIndex> inputVariables;
    std::vector<VarIndex> outputVariables;
    std::vector<std::size_t> layerSizes;
    std::vector<std::pair<VarIndex, VarIndex>> reluList;
    std::vector<Equation> structuralEquations;
    std::unordered_map<VarIndex, float> hardInputLowerBounds;
    std::unordered_map<VarIndex, float> hardInputUpperBounds;

    std::unordered_map<VarIndex, float> lowerBounds;
    std::unordered_map<VarIndex, float> upperBounds;
    std::vector<Equation> extraEquations;

};
}

class MarabouVerifier::MarabouImpl {
public:
    void loadModel(NNet const & network);

    void addUpperBound(LayerIndex layer, NodeIndex var, float value);
    void addLowerBound(LayerIndex layer, NodeIndex var, float value);

    void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs);

    Answer check();

    void clearAdditionalConstraints();

private:
    std::unique_ptr<QueryIncrementalWrapper> queryWrapper;
};

MarabouVerifier::MarabouVerifier() { pimpl = new MarabouImpl(); }

MarabouVerifier::~MarabouVerifier() { delete pimpl; }

void MarabouVerifier::loadModel(NNet const & network) {
    pimpl->loadModel(network);
}

void MarabouVerifier::addUpperBound(LayerIndex layer, NodeIndex var, float value) {
    pimpl->addUpperBound(layer, var, value);
}

void MarabouVerifier::addLowerBound(LayerIndex layer, NodeIndex var, float value) {
    pimpl->addLowerBound(layer, var, value);
}

void MarabouVerifier::addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) {
    pimpl->addConstraint(layer, lhs, rhs);
}

Verifier::Answer MarabouVerifier::check() {
    return pimpl->check();
}

void MarabouVerifier::clearAdditionalConstraints() {
    pimpl->clearAdditionalConstraints();
}

/*
 * Actual implementation
 */

namespace {
VarIndex QueryIncrementalWrapper::registerNewVariable() {
    return numVars++;
}

VarIndex QueryIncrementalWrapper::getVarIndex(LayerIndex layerNum, NodeIndex nodeIndex, VariableType type) const {
    if (layerNum == 0) {
        assert(type == VariableType::FORWARD);
        return nodeIndex;
    }
    std::size_t offset = layerSizes[0];
    for (std::size_t i = 1; i < layerNum; ++i) {
        offset += layerSizes[i] * 2; // For both backward and forward variables
    }
    switch (type) {
        case VariableType::BACKWARD:
            return offset + nodeIndex;
        case VariableType::FORWARD:
            return offset + layerSizes[layerNum] + nodeIndex;
    }
}

void QueryIncrementalWrapper::markInputVariable(VarIndex var) {
    inputVariables.push_back(var);
}

void QueryIncrementalWrapper::markOutputVariable(VarIndex var) {
    outputVariables.push_back(var);
}

void QueryIncrementalWrapper::setLowerBound(LayerIndex layer, NodeIndex node, float val) {
    auto var = getVarIndex(layer, node, layer == 0 ? VariableType::FORWARD : VariableType::BACKWARD);
    lowerBounds[var] = val;
}

void QueryIncrementalWrapper::setUpperBound(LayerIndex layer, NodeIndex node, float val) {
    auto var = getVarIndex(layer, node, layer == 0 ? VariableType::FORWARD : VariableType::BACKWARD);
    upperBounds[var] = val;
}

void QueryIncrementalWrapper::setHardInputLowerBound(VarIndex var, float val) {
    hardInputLowerBounds[var] = val;
}

void QueryIncrementalWrapper::setHardInputUpperBound(VarIndex var, float val) {
    hardInputUpperBounds[var] = val;
}

std::unique_ptr<QueryIncrementalWrapper> QueryIncrementalWrapper::fromNNet(NNet const & network) {
    auto queryWrapper = std::make_unique<QueryIncrementalWrapper>();
    assert(network.getNumLayers() >= 2);
    auto & layerSizes = queryWrapper->layerSizes;
    for (std::size_t layerNum = 0; layerNum < network.getNumLayers(); ++layerNum) {
        layerSizes.push_back(network.getLayerSize(layerNum));
    }

    // Register forward variables for input layer
    for (std::size_t node = 0; node < layerSizes[0]; ++node) {
        auto var = queryWrapper->registerNewVariable();
        queryWrapper->markInputVariable(var);
    }
    // Register backward and forward variables for hidden layers
    for (std::size_t layerNum = 0; layerNum < network.getNumLayers(); ++layerNum) {
        for (std::size_t node = 0; node < layerSizes[layerNum]; ++node) {
            queryWrapper->registerNewVariable();
            queryWrapper->registerNewVariable();
        }
    }
    // Register backward variables for output layer
    for (std::size_t node = 0; node < layerSizes.back(); ++node) {
        auto var = queryWrapper->registerNewVariable();
        queryWrapper->markOutputVariable(var);
    }

    // Build equations for each variable from weights and biases
    // RHS variables start from first hidden layer, i.e., we skip input layer
    for (std::size_t layerNum = 1; layerNum < network.getNumLayers(); ++layerNum) {
        for (std::size_t node = 0; node < network.getLayerSize(layerNum); ++node) {
            Equation eq;
            auto const & weights = network.getWeights(layerNum, node);
            assert(weights.size() == network.getLayerSize(layerNum - 1));
            for (std::size_t incomingIndex = 0; incomingIndex < weights.size(); ++incomingIndex) {
                auto var = queryWrapper->getVarIndex(layerNum - 1, incomingIndex, VariableType::FORWARD);
                eq.addAddend(weights[incomingIndex], var);
            }
            eq.addAddend(-1.0, queryWrapper->getVarIndex(layerNum, node, VariableType::BACKWARD));
            eq.setScalar(-network.getBias(layerNum, node));
            queryWrapper->addStructuralEquation(std::move(eq));
        }
    }
    // Add ReLUs
    for (std::size_t layerNum = 1; layerNum < network.getNumLayers() - 1; ++layerNum) {
        for (std::size_t node = 0; node < network.getLayerSize(layerNum); ++node) {
            queryWrapper->reluList.emplace_back(queryWrapper->getVarIndex(layerNum, node, VariableType::BACKWARD),
                                                queryWrapper->getVarIndex(layerNum, node, VariableType::FORWARD)
            );
        }
    }

    // Add input lower and upper bounds
    for (std::size_t node = 0; node < layerSizes[0]; ++node) {
        queryWrapper->setHardInputLowerBound(queryWrapper->getVarIndex(0, node, VariableType::FORWARD), network.getInputLowerBound(node));
        queryWrapper->setHardInputUpperBound(queryWrapper->getVarIndex(0, node, VariableType::FORWARD), network.getInputUpperBound(node));
    }
    // TODO: Add lower and upper bounds on input

    return queryWrapper;
}

std::unique_ptr<InputQuery> QueryIncrementalWrapper::buildQuery() const {
    auto query = std::make_unique<InputQuery>();

    query->setNumberOfVariables(numVars);

    for (std::size_t i = 0; i < inputVariables.size(); i++) {
        query->markInputVariable(inputVariables[i], i);
    }

    for (std::size_t i = 0; i < outputVariables.size(); i++) {
        query->markOutputVariable(outputVariables[i], i);
    }

    for (auto const & eq : structuralEquations) {
        query->addEquation(eq);
    }

    for (auto && [var, val] : hardInputLowerBounds) {
        query->setLowerBound(var, val);
    }

    for (auto && [var, val] : hardInputUpperBounds) {
        query->setUpperBound(var, val);
    }

    for (auto const & eq : extraEquations) {
        query->addEquation(eq);
    }

    for (auto && [incoming, outgoing] : reluList) {
        // TODO: Check with Marabou, this looks like memory leak
        query->addPiecewiseLinearConstraint(new ReluConstraint(incoming, outgoing));
    }

    for (auto && [var, val] : lowerBounds) {
        query->setLowerBound(var, val);
    }

    for (auto && [var, val] : upperBounds) {
        query->setUpperBound(var, val);
    }

    return query;
}

void QueryIncrementalWrapper::clearAdditionalConstraints() {
    extraEquations.clear();
    lowerBounds.clear();
    upperBounds.clear();
}

}

void MarabouVerifier::MarabouImpl::loadModel(const xai::NNet & network) {
    queryWrapper = QueryIncrementalWrapper::fromNNet(network);
}

void MarabouVerifier::MarabouImpl::clearAdditionalConstraints() {
    queryWrapper->clearAdditionalConstraints();
}

namespace{
Verifier::Answer toAnswer(Engine::ExitCode exitCode) {
    switch (exitCode) {
        case Engine::ExitCode::SAT:
            return Verifier::Answer::SAT;
        case Engine::ExitCode::UNSAT:
            return Verifier::Answer::UNSAT;
        case Engine::ExitCode::UNKNOWN:
            return Verifier::Answer::UNKNOWN;
        case Engine::ExitCode::ERROR:
            return Verifier::Answer::ERROR;
        default:
            return Verifier::Answer::UNKNOWN;
    }
}
}

Verifier::Answer MarabouVerifier::MarabouImpl::check() {
    auto queryPtr = queryWrapper->buildQuery();
    auto & query = *queryPtr;
    Engine engine;
    bool continueWithSolving = engine.processInputQuery(query, true);
    if (not continueWithSolving) {
        return toAnswer(engine.getExitCode());
    }
    bool feasible = engine.solve();
    auto exitCode = engine.getExitCode();
    assert(feasible == (exitCode == Engine::ExitCode::SAT));
    return toAnswer(exitCode);
}

void MarabouVerifier::MarabouImpl::addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs,
                                         float rhs) {

}

void MarabouVerifier::MarabouImpl::addUpperBound(LayerIndex layer, NodeIndex node, float value) {
    queryWrapper->setUpperBound(layer, node, value);

}

void MarabouVerifier::MarabouImpl::addLowerBound(LayerIndex layer, NodeIndex node, float value) {
    queryWrapper->setLowerBound(layer, node, value);
}

} // namespace xai::verifiers
