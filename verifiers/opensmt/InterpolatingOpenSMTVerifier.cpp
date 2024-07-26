#include "InterpolatingOpenSMTVerifier.h"

#include "ArithLogic.h"
#include "LogicFactory.h"
#include "MainSolver.h"
#include "StringConv.h"

namespace xai::verifiers {

namespace {

class NNFTransformer {
    Logic & logic;

    std::unordered_map<PTRef, PTRef, PTRefHash> transformed;
    std::unordered_map<PTRef, PTRef, PTRefHash> negated;

    PTRef transform(PTRef);
    PTRef negate(PTRef);

public:
    NNFTransformer(Logic & logic) : logic(logic) {}

    PTRef toNNF(PTRef fla) { return transform(fla); };
};

PTRef NNFTransformer::transform(PTRef fla) {
    auto it = transformed.find(fla);
    if (it != transformed.end()) {
        return it->second;
    }
    if (logic.isAtom(fla)) {
        transformed.insert({fla, fla});
        return fla;
    }
    if (logic.isAnd(fla)) {
        auto size = logic.getPterm(fla).size();
        vec<PTRef> nargs;
        nargs.capacity(size);
        for (int i = 0; i < size; ++i) {
            PTRef child = logic.getPterm(fla)[i];
            nargs.push(transform(child));
        }
        PTRef nfla = logic.mkAnd(nargs);
        transformed.insert({fla, nfla});
        return nfla;
    }
    if (logic.isOr(fla)) {
        auto size = logic.getPterm(fla).size();
        vec<PTRef> nargs;
        nargs.capacity(size);
        for (int i = 0; i < size; ++i) {
            PTRef child = logic.getPterm(fla)[i];
            nargs.push(transform(child));
        }
        PTRef nfla = logic.mkOr(nargs);
        transformed.insert({fla, nfla});
        return nfla;
    }
    if (logic.isNot(fla)) {
        PTRef npos = transform(logic.getPterm(fla)[0]);
        PTRef nfla = negate(npos);
        transformed.insert({fla, nfla});
        return nfla;
    }
    if (logic.getSymRef(fla) == logic.getSym_eq()) { // Boolean equality
        // a <=> b iff (~a or b) and (~b or a)
        assert(logic.getPterm(fla).size() == 2);
        PTRef firstTransformed = transform(logic.getPterm(fla)[0]);
        PTRef secondTransformed = transform(logic.getPterm(fla)[1]);
        PTRef firstTransformNegated = negate(firstTransformed);
        PTRef secondTransformNegated = negate(secondTransformed);
        PTRef nfla = logic.mkAnd(
            logic.mkOr(firstTransformNegated, secondTransformed),
            logic.mkOr(secondTransformNegated, firstTransformed)
        );
        transformed.insert({fla, nfla});
        return nfla;
    }
    assert(false);
    throw std::logic_error("Unexpected formula in NNF transformation");
}

PTRef NNFTransformer::negate(PTRef fla) {
    assert(logic.isAnd(fla) or logic.isOr(fla) or logic.isAtom(fla) or (logic.isNot(fla) and logic.isAtom(logic.getPterm(fla)[0])));
    auto it = negated.find(fla);
    if (it != negated.end()) {
        return it->second;
    }
    if (logic.isNot(fla)) {
        assert(logic.isAtom(logic.getPterm(fla)[0]));
        PTRef nfla = logic.getPterm(fla)[0];
        negated.insert({fla, nfla});
        return nfla;
    }
    if (logic.isAtom(fla)) {
        PTRef nfla = logic.mkNot(fla);
        negated.insert({fla, nfla});
        return nfla;
    }
    if (logic.isAnd(fla)) {
        auto size = logic.getPterm(fla).size();
        vec<PTRef> nargs;
        nargs.capacity(size);
        for (int i = 0; i < size; ++i) {
            PTRef child = logic.getPterm(fla)[i];
            nargs.push(negate(child));
        }
        PTRef nfla = logic.mkOr(nargs);
        negated.insert({fla, nfla});
        return nfla;
    }
    if (logic.isOr(fla)) {
        auto size = logic.getPterm(fla).size();
        vec<PTRef> nargs;
        nargs.capacity(size);
        for (int i = 0; i < size; ++i) {
            PTRef child = logic.getPterm(fla)[i];
            nargs.push(negate(child));
        }
        PTRef nfla = logic.mkAnd(nargs);
        negated.insert({fla, nfla});
        return nfla;
    }
    assert(false);
    throw std::logic_error("Unexpected formula in NNF transformation");
}

} // namespace


class InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl {
public:
    void loadModel(NNet const & network);

    void addUpperBound(LayerIndex layer, NodeIndex var, float value);
    void addLowerBound(LayerIndex layer, NodeIndex var, float value);
    void addClassificationConstraint(NodeIndex node, float threshold);

    void addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs);

    Answer check();

    void pushCheckpoint();
    void popCheckpoint();

    Explanation explain();

private:
    void resetSolver();

    std::unique_ptr<ArithLogic> logic;
    std::unique_ptr<MainSolver> solver;
    std::unique_ptr<SMTConfig> config;
    std::vector<PTRef> inputVars;
    std::vector<PTRef> outputVars;
    std::vector<std::size_t> layerSizes;

    std::size_t assertionCounter = 0;
    std::optional<std::size_t> assertionCounterAtCheckpoint;
    sstat lastAnswer = s_Undef;

};

InterpolatingOpenSMTVerifier::InterpolatingOpenSMTVerifier() { pimpl = new InterpolatingOpenSMTImpl(); }

InterpolatingOpenSMTVerifier::~InterpolatingOpenSMTVerifier() { delete pimpl; }

void InterpolatingOpenSMTVerifier::loadModel(NNet const & network) {
    pimpl->loadModel(network);
}

void InterpolatingOpenSMTVerifier::addUpperBound(LayerIndex layer, NodeIndex var, float value) {
    pimpl->addUpperBound(layer, var, value);
}

void InterpolatingOpenSMTVerifier::addLowerBound(LayerIndex layer, NodeIndex var, float value) {
    pimpl->addLowerBound(layer, var, value);
}

void InterpolatingOpenSMTVerifier::addClassificationConstraint(NodeIndex node, float threshold=0) {
    pimpl->addClassificationConstraint(node, threshold);
}

void InterpolatingOpenSMTVerifier::addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) {
    pimpl->addConstraint(layer, lhs, rhs);
}

InterpolatingVerifier::Answer InterpolatingOpenSMTVerifier::check() {
    return pimpl->check();
}

void InterpolatingOpenSMTVerifier::pushCheckpoint() {
    pimpl->pushCheckpoint();
}

void InterpolatingOpenSMTVerifier::popCheckpoint() {
    pimpl->popCheckpoint();
}

InterpolatingVerifier::Explanation InterpolatingOpenSMTVerifier::explain() {
    return pimpl->explain();
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

InterpolatingVerifier::Answer toAnswer(sstat res) {
    if (res == s_False)
        return InterpolatingVerifier::Answer::UNSAT;
    if (res == s_True)
        return InterpolatingVerifier::Answer::SAT;
    if (res == s_Error)
        return InterpolatingVerifier::Answer::ERROR;
    if (res == s_Undef)
        return InterpolatingVerifier::Answer::UNKNOWN;
    return InterpolatingVerifier::Answer::UNKNOWN;
}
}

void InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl::loadModel(NNet const & network) {
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
    assertionCounter++;
}

void InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl::addUpperBound(LayerIndex layer, NodeIndex node, float value) {
    if (layer != 0 and layer != layerSizes.size() - 1)
        throw std::logic_error("Unimplemented!");
    PTRef var = layer == 0 ? inputVars.at(node) : outputVars.at(node);
    solver->insertFormula(logic->mkLeq(var, logic->mkRealConst(floatToRational(value))));
    ++assertionCounter;
}

void InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl::addLowerBound(LayerIndex layer, NodeIndex node, float value) {
    if (layer != 0 and layer != layerSizes.size() - 1)
        throw std::logic_error("Unimplemented!");
    PTRef var = layer == 0 ? inputVars.at(node) : outputVars.at(node);
    solver->insertFormula(logic->mkGeq(var, logic->mkRealConst(floatToRational(value))));
    ++assertionCounter;
}

void InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl::addClassificationConstraint(NodeIndex node, float threshold=0.0){
    // Ensure the node index is within the range of outputVars
    if (node >= outputVars.size()) {
        throw std::out_of_range("Node index is out of range for outputVars.");
    }

    PTRef targetNodeVar = outputVars[node];
    std::vector<PTRef> constraints;

    for (size_t i = 0; i < outputVars.size(); ++i) {
        if (i != node) {
            // Create a constraint: (targetNodeVar - outputVars[i]) > threshold
            PTRef diff = logic->mkMinus(targetNodeVar, outputVars[i]);
            PTRef thresholdConst = logic->mkRealConst(floatToRational(threshold));
            PTRef constraint = logic->mkGt(diff, thresholdConst);
            constraints.push_back(constraint);
        }
    }

    if (!constraints.empty()) {
        PTRef combinedConstraint = logic->mkAnd(constraints);
        PTRef negatedProperty = logic->mkNot(combinedConstraint);
        solver->insertFormula(negatedProperty);
        ++assertionCounter;
    }
}

void
InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl::addConstraint(LayerIndex layer, std::vector<std::pair<NodeIndex, int>> lhs, float rhs) {
    throw std::logic_error("Unimplemented!");
}

InterpolatingVerifier::Answer InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl::check() {
    lastAnswer = solver->check();
    return toAnswer(lastAnswer);
}

void InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl::resetSolver() {
    config = std::make_unique<SMTConfig>();
    const char * msg = nullptr;
    config->setOption(SMTConfig::o_produce_inter, SMTOption("true"), msg);
    config->setOption(SMTConfig::o_simplify_inter, SMTOption(4), msg);
    config->setLRAInterpolationAlgorithm(itp_lra_alg_decomposing_strong);
    logic = std::make_unique<ArithLogic>(opensmt::Logic_t::QF_LRA);
    solver = std::make_unique<MainSolver>(*logic, *config, "verifier");
    inputVars.clear();
    outputVars.clear();
}

void InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl::pushCheckpoint() {
    assertionCounterAtCheckpoint = assertionCounter;
    solver->push();
}

void InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl::popCheckpoint() {
    solver->pop();
}

InterpolatingVerifier::Explanation InterpolatingOpenSMTVerifier::InterpolatingOpenSMTImpl::explain() {
    if (lastAnswer != s_False) { return {}; }
    if (not assertionCounterAtCheckpoint) { return {}; }

    auto itpContext = solver->getInterpolationContext();
    ipartitions_t partitions;
    for (unsigned i = 0; i < assertionCounterAtCheckpoint.value(); ++i) {
        opensmt::setbit(partitions, i);
    }
    vec<PTRef> itps;

    itpContext->getSingleInterpolant(itps, partitions);
    PTRef inputInterpolant = logic->mkNot(itps[0]);
    inputInterpolant = NNFTransformer(*logic).toNNF(inputInterpolant);
    std::cout << logic->pp(inputInterpolant) << std::endl;
    exit(1);
}

} // namespace xai::verifiers