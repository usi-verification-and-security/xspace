#include "OpenSMTVerifier.h"

#include <api/MainSolver.h>
#include <common/StringConv.h>
#include <logics/ArithLogic.h>
#include <logics/LogicFactory.h>

#include <algorithm>
#include <string>
#include <unordered_map>

namespace xai::verifiers {

using namespace opensmt;

namespace { // Helper methods
FastRational floatToRational(float value);
}

class OpenSMTVerifier::OpenSMTImpl {
public:
    void loadModel(nn::NNet const & network);

    PTRef makeUpperBound(LayerIndex layer, NodeIndex node, float value) {
        return makeUpperBound(layer, node, floatToRational(value));
    }
    PTRef makeLowerBound(LayerIndex layer, NodeIndex node, float value) {
        return makeLowerBound(layer, node, floatToRational(value));
    }
    PTRef makeEquality(LayerIndex layer, NodeIndex node, float value) {
        return makeEquality(layer, node, floatToRational(value));
    }
    PTRef makeUpperBound(LayerIndex layer, NodeIndex node, FastRational value);
    PTRef makeLowerBound(LayerIndex layer, NodeIndex node, FastRational value);
    PTRef makeEquality(LayerIndex layer, NodeIndex node, FastRational value);

    PTRef addUpperBound(LayerIndex layer, NodeIndex node, float value, bool explanationTerm = false);
    PTRef addLowerBound(LayerIndex layer, NodeIndex node, float value, bool explanationTerm = false);
    PTRef addEquality(LayerIndex layer, NodeIndex node, float value, bool explanationTerm = false);

    void addClassificationConstraint(NodeIndex node, float threshold);

    void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs);

    void init();

    void push();
    void pop();

    Answer check();

    void resetSampleQuery();
    void resetSample();
    void reset();

    UnsatCore getUnsatCore() const;

    opensmt::MainSolver const & getSolver() const { return *solver; }
    opensmt::MainSolver & getSolver() { return *solver; }

private:
    static std::string makeTermName(LayerIndex layer, NodeIndex node, std::string prefix = "") {
        assert(layer == 0);
        return prefix + "n" + std::to_string(node);
    }

    bool containsInputLowerBound(PTRef term) const { return inputVarLowerBoundToIndex.contains(term); }
    bool containsInputUpperBound(PTRef term) const { return inputVarUpperBoundToIndex.contains(term); }
    bool containsInputEquality(PTRef term) const { return inputVarEqualityToIndex.contains(term); }
    NodeIndex nodeIndexOfInputLowerBound(PTRef term) const { return inputVarLowerBoundToIndex.at(term); }
    NodeIndex nodeIndexOfInputUpperBound(PTRef term) const { return inputVarUpperBoundToIndex.at(term); }
    NodeIndex nodeIndexOfInputEquality(PTRef term) const { return inputVarEqualityToIndex.at(term); }

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

OpenSMTVerifier::OpenSMTVerifier() : pimpl{std::make_unique<OpenSMTImpl>()} {}

OpenSMTVerifier::~OpenSMTVerifier() {}

void OpenSMTVerifier::loadModel(nn::NNet const & network) {
    pimpl->loadModel(network);
}

void OpenSMTVerifier::addUpperBound(LayerIndex layer, NodeIndex var, float value, bool explanationTerm) {
    pimpl->addUpperBound(layer, var, value, explanationTerm);
}

void OpenSMTVerifier::addLowerBound(LayerIndex layer, NodeIndex var, float value, bool explanationTerm) {
    pimpl->addLowerBound(layer, var, value, explanationTerm);
}

void OpenSMTVerifier::addEquality(LayerIndex layer, NodeIndex var, float value, bool explanationTerm) {
    pimpl->addEquality(layer, var, value, explanationTerm);
}

void OpenSMTVerifier::addClassificationConstraint(NodeIndex node, float threshold=0) {
    pimpl->addClassificationConstraint(node, threshold);
}

void OpenSMTVerifier::addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) {
    pimpl->addConstraint(layer, lhs, rhs);
}

void OpenSMTVerifier::initImpl() {
    pimpl->init();
}

void OpenSMTVerifier::push() {
    pimpl->push();
}

void OpenSMTVerifier::pop() {
    pimpl->pop();
}

Verifier::Answer OpenSMTVerifier::checkImpl() {
    return pimpl->check();
}

void OpenSMTVerifier::resetSampleQuery() {
    pimpl->resetSampleQuery();
    UnsatCoreVerifier::resetSampleQuery();
}

void OpenSMTVerifier::resetSample() {
    pimpl->resetSample();
    UnsatCoreVerifier::resetSample();
}

void OpenSMTVerifier::reset() {
    pimpl->reset();
    UnsatCoreVerifier::reset();
}

UnsatCore OpenSMTVerifier::getUnsatCore() const {
    return pimpl->getUnsatCore();
}

opensmt::MainSolver const & OpenSMTVerifier::getSolver() const {
    return pimpl->getSolver();
}

opensmt::MainSolver & OpenSMTVerifier::getSolver() {
    return pimpl->getSolver();
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

void OpenSMTVerifier::OpenSMTImpl::loadModel(nn::NNet const & network) {
    // create input variables
    for (NodeIndex i = 0u; i < network.getLayerSize(0); ++i) {
        auto name = "x" + std::to_string(i + 1);
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

PTRef OpenSMTVerifier::OpenSMTImpl::makeEquality(LayerIndex layer, NodeIndex node, FastRational value) {
    PTRef lterm = makeLowerBound(layer, node, value);
    PTRef uterm = makeUpperBound(layer, node, std::move(value));
    return logic->mkAnd(lterm, uterm);
}

PTRef OpenSMTVerifier::OpenSMTImpl::addUpperBound(LayerIndex layer, NodeIndex node, float value, bool explanationTerm) {
    PTRef term = makeUpperBound(layer, node, value);
    solver->insertFormula(term);
    if (not explanationTerm) { return term; }

    solver->getTermNames().insert(makeTermName(layer, node, "u_"), term);
    auto const [_, inserted] = inputVarUpperBoundToIndex.emplace(term, node);
    assert(inserted);
    return term;
}

PTRef OpenSMTVerifier::OpenSMTImpl::addLowerBound(LayerIndex layer, NodeIndex node, float value, bool explanationTerm) {
    PTRef term = makeLowerBound(layer, node, value);
    solver->insertFormula(term);
    if (not explanationTerm) { return term; }

    solver->getTermNames().insert(makeTermName(layer, node, "l_"), term);
    auto const [_, inserted] = inputVarLowerBoundToIndex.emplace(term, node);
    assert(inserted);
    return term;
}

PTRef OpenSMTVerifier::OpenSMTImpl::addEquality(LayerIndex layer, NodeIndex node, float value, bool explanationTerm) {
    PTRef term = makeEquality(layer, node, value);
    solver->insertFormula(term);
    if (not explanationTerm) { return term; }

    solver->getTermNames().insert(makeTermName(layer, node, "e_"), term);
    auto const [_, inserted] = inputVarEqualityToIndex.emplace(term, node);
    assert(inserted);
    return term;
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

void OpenSMTVerifier::OpenSMTImpl::push() {
    solver->push();
}

void OpenSMTVerifier::OpenSMTImpl::pop() {
    solver->pop();
}

Verifier::Answer OpenSMTVerifier::OpenSMTImpl::check() {
    auto res = solver->check();
    return toAnswer(res);
}

void OpenSMTVerifier::OpenSMTImpl::init() {
    config = std::make_unique<SMTConfig>();
    const char* msg = "ok";
    //+ make configurable
    config->setOption(SMTConfig::o_produce_unsat_cores, SMTOption(true), msg);
    config->setOption(SMTConfig::o_minimal_unsat_cores, SMTOption(true), msg);
    config->setOption(SMTConfig::o_produce_inter, SMTOption(true), msg);
    config->setLRAInterpolationAlgorithm(itp_lra_alg_weak);
    config->setReduction(true);
    config->setSimplifyInterpolant(4);

    // reset() is called by Verifier
}

void OpenSMTVerifier::OpenSMTImpl::resetSampleQuery() {
    inputVarLowerBoundToIndex.clear();
    inputVarUpperBoundToIndex.clear();
    inputVarEqualityToIndex.clear();
}

void OpenSMTVerifier::OpenSMTImpl::resetSample() {
    // resetSampleQuery() is called by Verifier
}

void OpenSMTVerifier::OpenSMTImpl::reset() {
    logic = std::make_unique<ArithLogic>(opensmt::Logic_t::QF_LRA);
    solver = std::make_unique<MainSolver>(*logic, *config, "verifier");
    inputVars.clear();
    outputVars.clear();

    // resetSample() is called by Verifier
}

UnsatCore OpenSMTVerifier::OpenSMTImpl::getUnsatCore() const {
    auto const unsatCore = solver->getUnsatCore();

    UnsatCore unsatCoreRes;

    for (PTRef term : unsatCore->getTerms()) {
        bool const containsEquality = containsInputEquality(term);
        if (containsEquality) {
            unsatCoreRes.equalities.push_back(nodeIndexOfInputEquality(term));
            continue;
        }

        bool const containsLower = containsInputLowerBound(term);
        if (containsLower) {
            unsatCoreRes.lowerBounds.push_back(nodeIndexOfInputLowerBound(term));
            continue;
        }

        assert(containsInputUpperBound(term));
        unsatCoreRes.upperBounds.push_back(nodeIndexOfInputUpperBound(term));
    }

    std::ranges::sort(unsatCoreRes.equalities);
    std::ranges::sort(unsatCoreRes.lowerBounds);
    std::ranges::sort(unsatCoreRes.upperBounds);

    return unsatCoreRes;
}

} // namespace xai::verifiers
