#include "OpenSMTVerifier.h"

#include "experiments/Config.h"

#include "ArithLogic.h"
#include "LogicFactory.h"
#include "MainSolver.h"
#include "StringConv.h"

namespace xai::verifiers {

using namespace opensmt;

using experiments::Config;

namespace { // Helper methods
FastRational floatToRational(float value);
}

class OpenSMTVerifier::OpenSMTImpl {
public:
    void loadModel(NNet const & network);

    PTRef makeUpperBound(LayerIndex layer, NodeIndex node, float value) {
        return makeUpperBound(layer, node, floatToRational(value));
    }
    PTRef makeLowerBound(LayerIndex layer, NodeIndex node, float value) {
        return makeLowerBound(layer, node, floatToRational(value));
    }
    PTRef makeUpperBound(LayerIndex layer, NodeIndex node, FastRational value);
    PTRef makeLowerBound(LayerIndex layer, NodeIndex node, FastRational value);

    PTRef addUpperBound(LayerIndex layer, NodeIndex node, float value);
    PTRef addLowerBound(LayerIndex layer, NodeIndex node, float value);
    void addEquality(LayerIndex layer, NodeIndex node, float value);
    void addClassificationConstraint(NodeIndex node, float threshold);

    void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs);

    Answer check();

    void clearAdditionalConstraints();

private:
    void resetSolver();

    std::unique_ptr<ArithLogic> logic;
    std::unique_ptr<MainSolver> solver;
    std::unique_ptr<SMTConfig> config;
    std::vector<PTRef> inputVars;
    std::vector<PTRef> outputVars;
    std::vector<std::size_t> layerSizes;

};

OpenSMTVerifier::OpenSMTVerifier() { pimpl = new OpenSMTImpl(); }

OpenSMTVerifier::~OpenSMTVerifier() { delete pimpl; }

void OpenSMTVerifier::loadModel(NNet const & network) {
    pimpl->loadModel(network);
}

void OpenSMTVerifier::addUpperBound(LayerIndex layer, NodeIndex var, float value) {
    pimpl->addUpperBound(layer, var, value);
}

void OpenSMTVerifier::addLowerBound(LayerIndex layer, NodeIndex var, float value) {
    pimpl->addLowerBound(layer, var, value);
}

void OpenSMTVerifier::addEquality(LayerIndex layer, NodeIndex var, float value) {
    pimpl->addEquality(layer, var, value);
}

void OpenSMTVerifier::addClassificationConstraint(NodeIndex node, float threshold=0) {
    pimpl->addClassificationConstraint(node, threshold);
}

void OpenSMTVerifier::addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) {
    pimpl->addConstraint(layer, lhs, rhs);
}

Verifier::Answer OpenSMTVerifier::check() {
    return pimpl->check();
}

void OpenSMTVerifier::clearAdditionalConstraints() {
    pimpl->clearAdditionalConstraints();
}

/*
 * Actual implementation
 */

namespace { // Helper methods
FastRational floatToRational(float value) {
    auto s = std::to_string(value);
    char* rationalString;
    opensmt::stringToRational(rationalString, s.c_str());
    auto res = FastRational(rationalString);
    free(rationalString);
    return res;
}

Verifier::Answer toAnswer(sstat res) {
    if (res == s_False)
        return Verifier::Answer::UNSAT;
    if (res == s_True)
        return Verifier::Answer::SAT;
    if (res == s_Error)
        return Verifier::Answer::ERROR;
    if (res == s_Undef)
        return Verifier::Answer::UNKNOWN;
    return Verifier::Answer::UNKNOWN;
}
}

void OpenSMTVerifier::OpenSMTImpl::loadModel(NNet const & network) {
    resetSolver();
    // create input variables
    for (NodeIndex i = 0u; i < network.getLayerSize(0); ++i) {
        auto name = "input" + std::to_string(i);
        PTRef var = logic->mkRealVar(name.c_str());
        inputVars.push_back(var);
    }

    // Create representation for each neuron in hidden layers, from input to output layers
    std::vector<PTRef> previousLayerRefs = inputVars;
    for (LayerIndex layer = 1u; layer < network.getNumLayers() - 1; layer++) {
        std::vector<PTRef> currentLayerRefs;
        for (NodeIndex node = 0u; node < network.getLayerSize(layer); ++node) {
            std::vector<PTRef> addends;
            float bias = network.getBias(layer, node);
            auto const & weights = network.getWeights(layer, node);
            PTRef biasTerm = logic->mkRealConst(floatToRational(bias));
            addends.push_back(biasTerm);

            assert(previousLayerRefs.size() == weights.size());
            for (int j = 0; j < weights.size(); j++) {
                PTRef weightTerm = logic->mkRealConst(floatToRational(weights[j]));
                PTRef addend = logic->mkTimes(weightTerm, previousLayerRefs[j]);
                addends.push_back(addend);
            }
            PTRef input = logic->mkPlus(addends);
            PTRef relu = logic->mkIte(logic->mkGeq(input, logic->getTerm_RealZero()), input, logic->getTerm_RealZero());
            currentLayerRefs.push_back(relu);
        }
        previousLayerRefs = std::move(currentLayerRefs);
    }

    // Create representation of the outputs (without RELU!)
    // TODO: Remove code duplication!
    auto lastLayerIndex = network.getNumLayers() - 1;
    auto lastLayerSize = network.getLayerSize(lastLayerIndex);
    outputVars.clear();
    for (NodeIndex node = 0u; node < lastLayerSize; ++node) {
        std::vector<PTRef> addends;
        float bias = network.getBias(lastLayerIndex, node);
        auto const & weights = network.getWeights(lastLayerIndex, node);
        PTRef biasTerm = logic->mkRealConst(floatToRational(bias));
        addends.push_back(biasTerm);

        assert(previousLayerRefs.size() == weights.size());
        for (int j = 0; j < weights.size(); j++) {
            PTRef weightTerm = logic->mkRealConst(floatToRational(weights[j]));
            PTRef addend = logic->mkTimes(weightTerm, previousLayerRefs[j]);
            addends.push_back(addend);
        }
        outputVars.push_back(logic->mkPlus(addends));
    }

    // Store information about layer sizes
    layerSizes.clear();
    for (LayerIndex layer = 0u; layer < network.getNumLayers(); layer++) {
        layerSizes.push_back(network.getLayerSize(layer));
    }

    // Collect hard bounds on inputs
    std::vector<PTRef> bounds;
    for (NodeIndex i = 0; i < network.getLayerSize(0); ++i) {
        float lb = network.getInputLowerBound(i);
        float ub = network.getInputUpperBound(i);
        bounds.push_back(logic->mkGeq(inputVars[i], logic->mkRealConst(floatToRational(lb))));
        bounds.push_back(logic->mkLeq(inputVars[i], logic->mkRealConst(floatToRational(ub))));
    }
    solver->insertFormula(logic->mkAnd(bounds));
    solver->push();
}

PTRef OpenSMTVerifier::OpenSMTImpl::makeUpperBound(LayerIndex layer, NodeIndex node, FastRational value) {
    if (layer != 0 and layer != layerSizes.size() - 1)
        throw std::logic_error("Unimplemented!");
    PTRef var = layer == 0 ? inputVars.at(node) : outputVars.at(node);
    return logic->mkLeq(var, logic->mkRealConst(value));
}

PTRef OpenSMTVerifier::OpenSMTImpl::makeLowerBound(LayerIndex layer, NodeIndex node, FastRational value) {
    if (layer != 0 and layer != layerSizes.size() - 1)
        throw std::logic_error("Unimplemented!");
    PTRef var = layer == 0 ? inputVars.at(node) : outputVars.at(node);
    return logic->mkGeq(var, logic->mkRealConst(value));
}

PTRef OpenSMTVerifier::OpenSMTImpl::addUpperBound(LayerIndex layer, NodeIndex node, float value) {
    PTRef term = makeUpperBound(layer, node, value);
    solver->insertFormula(term);
    return term;
}

PTRef OpenSMTVerifier::OpenSMTImpl::addLowerBound(LayerIndex layer, NodeIndex node, float value) {
    PTRef term = makeLowerBound(layer, node, value);
    solver->insertFormula(term);
    return term;
}

void OpenSMTVerifier::OpenSMTImpl::addEquality(LayerIndex layer, NodeIndex node, float value) {
    FastRational rat = floatToRational(value);
    PTRef lterm = makeLowerBound(layer, node, rat);
    PTRef uterm = makeUpperBound(layer, node, std::move(rat));
    PTRef term = logic->mkAnd(lterm, uterm);
    solver->insertFormula(term);
}

void OpenSMTVerifier::OpenSMTImpl::addClassificationConstraint(NodeIndex node, float threshold=0.0){
    // Ensure the node index is within the range of outputVars
    if (node >= outputVars.size()) {
        throw std::out_of_range("Node index is out of range for outputVars.");
    }

    PTRef targetNodeVar = outputVars[node];
    std::vector<PTRef> constraints;

    for (size_t i = 0; i < outputVars.size(); ++i) {
        if (i != node) {
            // Create a constraint: (targetNodeVar - outputVars[i]) > threshold
            PTRef diff = logic->mkMinus(outputVars[i], targetNodeVar);
            PTRef thresholdConst = logic->mkRealConst(floatToRational(threshold));
            PTRef constraint = logic->mkGt(diff, thresholdConst);
            constraints.push_back(constraint);
        }
    }

    if (!constraints.empty()) {
        PTRef combinedConstraint = logic->mkOr(constraints);
        solver->insertFormula(combinedConstraint);
    }
}

void
OpenSMTVerifier::OpenSMTImpl::addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) {
    throw std::logic_error("Unimplemented!");
}

Verifier::Answer OpenSMTVerifier::OpenSMTImpl::check() {
    auto res = solver->check();
    return toAnswer(res);
}

void OpenSMTVerifier::OpenSMTImpl::clearAdditionalConstraints() {
    solver->pop();
    solver->push();
}

void OpenSMTVerifier::OpenSMTImpl::resetSolver() {
    config = std::make_unique<SMTConfig>();
    logic = std::make_unique<ArithLogic>(opensmt::Logic_t::QF_LRA);
    solver = std::make_unique<MainSolver>(*logic, *config, "verifier");
    inputVars.clear();
    outputVars.clear();
}

} // namespace xai::verifiers
