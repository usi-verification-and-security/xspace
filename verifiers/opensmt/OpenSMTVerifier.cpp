#include "OpenSMTVerifier.h"

#include "ArithLogic.h"
#include "LogicFactory.h"
#include "MainSolver.h"

namespace xai::verifiers {

class OpenSMTVerifier::OpenSMTImpl {
public:
    void loadModel(NNet const & network);

    void addUpperBound(LayerIndex layer, NodeIndex var, float value);
    void addLowerBound(LayerIndex layer, NodeIndex var, float value);

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
    return FastRational(s.c_str());
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

    // Create representation for each neuron, from input to output layers
    std::vector<PTRef> previousLayerRefs = inputVars;
    for (LayerIndex layer = 1u; layer < network.getNumLayers(); layer++) {
        std::vector<PTRef> currentLayerRefs;
        for (NodeIndex node = 0u; node < network.getLayerSize(layer); ++node) {
            std::vector<PTRef> addends;
            float bias = network.getBias(layer, node);
            auto const & weights = network.getWeights(layer, node);
            PTRef biasTerm = logic->mkRealConst(FastRational(std::to_string(bias).c_str()));
            addends.push_back(logic->mkNeg(biasTerm));

            assert(previousLayerRefs.size() == weights.size());
            for (int j = 0; j < weights.size(); j++) {
                PTRef weightTerm = logic->mkRealConst(FastRational(std::to_string(weights[j]).c_str()));
                PTRef addend = logic->mkTimes(weightTerm, previousLayerRefs[j]);
                addends.push_back(addend);
            }
            PTRef input = logic->mkPlus(addends);
            PTRef relu = logic->mkIte(logic->mkGt(input, logic->getTerm_RealZero()), input, logic->getTerm_RealZero());
            currentLayerRefs.push_back(relu);
        }
        previousLayerRefs = currentLayerRefs;
    }
    outputVars = previousLayerRefs;

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

void OpenSMTVerifier::OpenSMTImpl::addUpperBound(LayerIndex layer, NodeIndex var, float value) {
    throw std::logic_error("Unimplemented!");
}

void OpenSMTVerifier::OpenSMTImpl::addLowerBound(LayerIndex layer, NodeIndex var, float value) {
    throw std::logic_error("Unimplemented!");
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