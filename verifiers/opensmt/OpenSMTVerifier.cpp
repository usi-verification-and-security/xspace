#include "OpenSMTVerifier.h"

#include "experiments/Config.h"

#include "ArithLogic.h"
#include "LogicFactory.h"
#include "MainSolver.h"
#include "StringConv.h"

#include <string>
#include <unordered_map>

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

    PTRef addUpperBound(LayerIndex layer, NodeIndex node, float value, bool namedTerm = false);
    PTRef addLowerBound(LayerIndex layer, NodeIndex node, float value, bool namedTerm = false);
    void addEquality(LayerIndex layer, NodeIndex node, float value, bool namedTerm = false);
    void addClassificationConstraint(NodeIndex node, float threshold);

    void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs);

    Answer check();

    void clearAdditionalConstraints();

    opensmt::MainSolver & getSolver() { return *solver; }

    bool containsInputLowerBound(PTRef term) const { return inputVarLowerBoundToIndex.contains(term); }
    bool containsInputUpperBound(PTRef term) const { return inputVarUpperBoundToIndex.contains(term); }
    bool containsInputEquality(PTRef term) const { return inputVarEqualityToIndex.contains(term); }
    NodeIndex nodeIndexOfInputLowerBound(PTRef term) const { return inputVarLowerBoundToIndex.at(term); }
    NodeIndex nodeIndexOfInputUpperBound(PTRef term) const { return inputVarUpperBoundToIndex.at(term); }
    NodeIndex nodeIndexOfInputEquality(PTRef term) const { return inputVarEqualityToIndex.at(term); }

private:
    void resetSolver();

    static std::string makeTermName(LayerIndex layer, NodeIndex node, std::string prefix = "") {
        assert(layer == 0);
        return prefix + "n" + std::to_string(node);
    }

    std::unique_ptr<ArithLogic> logic;
    std::unique_ptr<MainSolver> solver;
    std::unique_ptr<SMTConfig> config;
    std::vector<PTRef> inputVars;
    std::vector<PTRef> outputVars;
    std::vector<std::size_t> layerSizes;

    std::unordered_map<PTRef, NodeIndex, PTRefHash> inputVarLowerBoundToIndex;
    std::unordered_map<PTRef, NodeIndex, PTRefHash> inputVarUpperBoundToIndex;
    std::unordered_map<PTRef, NodeIndex, PTRefHash> inputVarEqualityToIndex;
};

OpenSMTVerifier::OpenSMTVerifier() { pimpl = new OpenSMTImpl(); }

OpenSMTVerifier::~OpenSMTVerifier() { delete pimpl; }

void OpenSMTVerifier::loadModel(NNet const & network) {
    pimpl->loadModel(network);
}

void OpenSMTVerifier::addUpperBound(LayerIndex layer, NodeIndex var, float value, bool namedTerm) {
    pimpl->addUpperBound(layer, var, value, namedTerm);
}

void OpenSMTVerifier::addLowerBound(LayerIndex layer, NodeIndex var, float value, bool namedTerm) {
    pimpl->addLowerBound(layer, var, value, namedTerm);
}

void OpenSMTVerifier::addEquality(LayerIndex layer, NodeIndex var, float value, bool namedTerm) {
    pimpl->addEquality(layer, var, value, namedTerm);
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

opensmt::MainSolver & OpenSMTVerifier::getSolver() {
    return pimpl->getSolver();
}

bool OpenSMTVerifier::containsInputLowerBound(opensmt::PTRef const & term) const {
    return pimpl->containsInputLowerBound(term);
}

bool OpenSMTVerifier::containsInputUpperBound(opensmt::PTRef const & term) const {
    return pimpl->containsInputUpperBound(term);
}

bool OpenSMTVerifier::containsInputEquality(opensmt::PTRef const & term) const {
    return pimpl->containsInputEquality(term);
}

NodeIndex OpenSMTVerifier::nodeIndexOfInputLowerBound(opensmt::PTRef const & term) const {
    return pimpl->nodeIndexOfInputLowerBound(term);
}

NodeIndex OpenSMTVerifier::nodeIndexOfInputUpperBound(opensmt::PTRef const & term) const {
    return pimpl->nodeIndexOfInputUpperBound(term);
}

NodeIndex OpenSMTVerifier::nodeIndexOfInputEquality(opensmt::PTRef const & term) const {
    return pimpl->nodeIndexOfInputEquality(term);
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

PTRef OpenSMTVerifier::OpenSMTImpl::addUpperBound(LayerIndex layer, NodeIndex node, float value, bool namedTerm) {
    PTRef term = makeUpperBound(layer, node, value);
    solver->insertFormula(term);
    if (not namedTerm) { return term; }

    solver->getTermNames().insert(makeTermName(layer, node, "u_"), term);
    auto const [_, inserted] = inputVarUpperBoundToIndex.emplace(term, node);
    assert(inserted);
    return term;
}

PTRef OpenSMTVerifier::OpenSMTImpl::addLowerBound(LayerIndex layer, NodeIndex node, float value, bool namedTerm) {
    PTRef term = makeLowerBound(layer, node, value);
    solver->insertFormula(term);
    if (not namedTerm) { return term; }

    solver->getTermNames().insert(makeTermName(layer, node, "l_"), term);
    auto const [_, inserted] = inputVarLowerBoundToIndex.emplace(term, node);
    assert(inserted);
    return term;
}

void OpenSMTVerifier::OpenSMTImpl::addEquality(LayerIndex layer, NodeIndex node, float value, bool namedTerm) {
    FastRational rat = floatToRational(value);
    PTRef lterm = makeLowerBound(layer, node, rat);
    PTRef uterm = makeUpperBound(layer, node, std::move(rat));
    PTRef term = logic->mkAnd(lterm, uterm);
    solver->insertFormula(term);
    if (not namedTerm) { return; }

    solver->getTermNames().insert(makeTermName(layer, node, "e_"), term);
    auto const [_, inserted] = inputVarEqualityToIndex.emplace(term, node);
    assert(inserted);
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
    const char* msg = "ok";
    config->setOption(SMTConfig::o_produce_unsat_cores, SMTOption(true), msg);
    if constexpr (Config::minimalUnsatCore) {
        config->setOption(SMTConfig::o_minimal_unsat_cores, SMTOption(true), msg);
    }
    if constexpr (Config::interpolation) {
        config->setOption(SMTConfig::o_produce_inter, SMTOption(true), msg);
        config->setLRAInterpolationAlgorithm(itp_lra_alg_weak);
        config->setReduction(true);
        config->setSimplifyInterpolant(4);
    }
    logic = std::make_unique<ArithLogic>(opensmt::Logic_t::QF_LRA);
    solver = std::make_unique<MainSolver>(*logic, *config, "verifier");
    inputVars.clear();
    outputVars.clear();
    inputVarLowerBoundToIndex.clear();
    inputVarUpperBoundToIndex.clear();
    inputVarEqualityToIndex.clear();
}

} // namespace xai::verifiers
