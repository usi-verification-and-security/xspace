#include "BasicVerix.h"

#include <experiments/Config.h>
#include <experiments/Utils.h>

#include <cassert>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <unordered_set>
#include <algorithm>

#include <verifiers/UnsatCoreVerifier.h>
#include <verifiers/opensmt/OpenSMTVerifier.h>
#include <api/MainSolver.h>

namespace xai::algo {

using namespace opensmt;

using experiments::Config;

namespace {
using input_t = std::vector<float>;

}

/*
 * Actual implementation
 */

BasicVerix::BasicVerix(std::string networkFile) {
    network = nn::NNet::fromFile(networkFile);
}


void BasicVerix::setVerifier(std::unique_ptr<Verifier> verifier) {
    this->verifier = std::move(verifier);
}

void BasicVerix::encodeClassificationConstraint(std::vector<float> const & output, NodeIndex label) {
    if (checksCount == 0) { std::cout << "computed output: "; }
    auto outputLayerIndex = network->getNumLayers() - 1;
        if (output.size() == 1) {
            // With single output, the flip in classification means flipping the value across certain threshold
            constexpr float THRESHOLD = 0;
            constexpr float PRECISION = 0.015625f;
            float outputValue = output[0];
            if (outputValue >= THRESHOLD) {
                if (checksCount == 0) { std::cout << 1; }
                verifier->addUpperBound(outputLayerIndex, 0, THRESHOLD - PRECISION);
            } else {
                if (checksCount == 0) { std::cout << 0; }
                verifier->addLowerBound(outputLayerIndex, 0, THRESHOLD + PRECISION);
            }
        } else {
            auto outputLayerSize = network->getLayerSize(outputLayerIndex);
            assert(outputLayerSize == output.size());
            // TODO: Build disjunction of inequalities
            verifier->addClassificationConstraint(label, 0);
            if (checksCount == 0) { std::cout << label; }
//            for (NodeIndex outputNode = 0; outputNode < outputLayerSize; ++outputNode) {
//                if (outputNode == label) { continue; }
//                throw std::logic_error("Not implemented yet!");
//            }
        }
    if (checksCount == 0) { std::cout << std::endl; }
}

verifiers::Verifier::Answer BasicVerix::check() {
    ++checksCount;
    return verifier->check();
}

BasicVerix::Result BasicVerix::computeExplanation(input_t const & inputValues, float freedom_factor,
                                                  std::vector<std::size_t> const & featureOrder) {
    checksCount = 0;
    assert(freedom_factor <= 1.0 and freedom_factor >= 0.0);
    if (not network or not verifier)
        return Result{};

    verifier->init();
    verifier->loadModel(*network);
    verifier->push();

    auto output = nn::computeOutput(inputValues, *network);
    NodeIndex label = std::max_element(output.begin(), output.end()) - output.begin();
    auto inputSize = network->getLayerSize(0);

    // Compute lower and upper bounds based on the freedom factor
    std::vector<float> inputLowerBounds;
    std::vector<float> inputUpperBounds;
    for (NodeIndex node = 0; node < inputSize; ++node) {
        float input_freedom = (network->getInputUpperBound(node) - network->getInputLowerBound(node)) * freedom_factor;
        inputLowerBounds.push_back(std::max(inputValues[node] - input_freedom, network->getInputLowerBound(node)));
        inputUpperBounds.push_back(std::min(inputValues[node] + input_freedom, network->getInputUpperBound(node)));
    }

    std::unordered_set<NodeIndex> explanationSet;
    std::unordered_set<NodeIndex> freeSet;
    assert(inputSize == inputValues.size());
    assert(featureOrder.size() == inputSize);
    for (NodeIndex nodeToConsider : featureOrder) {
        for (NodeIndex node = 0; node < inputSize; ++node) {
            if (node == nodeToConsider or freeSet.contains(node)) { // this input feature is free
                verifier->addLowerBound(0, node, inputLowerBounds[node]);
                verifier->addUpperBound(0, node, inputUpperBounds[node]);
            } else { // this input feature is fixed
                verifier->addEquality(0, node, inputValues[node]);
            }
        }
        // TODO: General property specification?
        // TODO: Figure out how to do this only once!
        encodeClassificationConstraint(output, label);
        auto answer = check();
        if (answer == Verifier::Answer::UNSAT) {
            freeSet.insert(nodeToConsider);
        } else {
            explanationSet.insert(nodeToConsider);
        }
        verifier->pop();
        verifier->push();
    }
    std::vector<NodeIndex> explanation{explanationSet.begin(), explanationSet.end()};
    std::sort(explanation.begin(), explanation.end());
    return Result{.explanation = std::move(explanation)};
}

BasicVerix::GeneralizedExplanation BasicVerix::computeGeneralizedExplanation(const std::vector<float> &inputValues,
                                       std::vector<std::size_t> const & featureOrder, int threshold) {
    checksCount = 0;
    auto output = nn::computeOutput(inputValues, *network);
    NodeIndex label = std::max_element(output.begin(), output.end()) - output.begin();
    float freedomFactor = 1.0f;
    auto result = computeExplanation(inputValues, freedomFactor, featureOrder);

    // Algorithm 1 in section 4.2 in the paper
    auto const & explanationFeatures = result.explanation;
    struct Bound { NodeIndex index; float value; };
    std::vector<Bound> lowerBounds;
    std::vector<Bound> upperBounds;
    for (auto index : explanationFeatures) {
        lowerBounds.push_back({.index = index, .value = inputValues.at(index)});
        upperBounds.push_back({.index = index, .value = inputValues.at(index)});
    }

    auto auxCheck = [&](){
        for (auto lb: lowerBounds) {
            verifier->addLowerBound(0, lb.index, lb.value);
        }
        for (auto ub: upperBounds) {
            verifier->addUpperBound(0, ub.index, ub.value);
        }
        #ifdef MARABOU
        // TODO: Figure out how to do this only once!
        this->encodeClassificationConstraint(output, label);
        #endif
        auto answer = check();
        #ifdef MARABOU
        verifier->pop();
        verifier->push();
        #endif
        return answer;
    };

    #ifndef MARABOU
    encodeClassificationConstraint(output, label);

    assert(dynamic_cast<verifiers::OpenSMTVerifier *>(verifier.get()));
    auto & openSMT = static_cast<verifiers::OpenSMTVerifier &>(*verifier);
    auto & solver = openSMT.getSolver();

    solver.push();
    #endif

    for (NodeIndex inputIndex : explanationFeatures) {

        float inputValue = inputValues.at(inputIndex);
        float lowerBound = network->getInputLowerBound(inputIndex);
        float upperBound = network->getInputUpperBound(inputIndex);

        if (lowerBound < inputValue) {
            // Try relaxing lower bound
            auto it = std::find_if(lowerBounds.begin(), lowerBounds.end(), [&](auto const & lb) { return lb.index == inputIndex; });
            it->value = lowerBound;
            auto answer = auxCheck();
            if (answer == Verifier::Answer::UNSAT) {
                lowerBounds.erase(it);
            } else {
                auto tmp = lowerBound;
                for (int i = 0 ; i < threshold; i++)
                {
                    tmp = (inputValue + tmp)/2;
                    it->value = tmp;
                    #ifdef MARABOU
                    answer = auxCheck();
                    #else
                    verifier->addLowerBound(0, it->index, it->value);
                    answer = check();
                    #endif
                    if (answer == Verifier::Answer::UNSAT) {
                        #ifdef MARABOU
                        it->value = tmp;
                        #endif
                        break;
                    }
                }

                if (answer != Verifier::Answer::UNSAT)
                    it->value = inputValue;
            }
            #ifndef MARABOU
            verifier->pop();
            verifier->push();
            #endif
        }

        if (upperBound > inputValue) {
            // Try relaxing upper bound
            auto it = std::find_if(upperBounds.begin(), upperBounds.end(), [&](auto const & ub) { return ub.index == inputIndex; });
            it->value = upperBound;
            auto answer = auxCheck();
            if (answer == Verifier::Answer::UNSAT) {
                upperBounds.erase(it);
            } else {
                auto tmp = upperBound;
                for (int i = 0 ; i < threshold; i++)
                {
                    tmp = (inputValue + tmp)/2;
                    it->value = tmp;
                    #ifdef MARABOU
                    answer = auxCheck();
                    #else
                    verifier->addUpperBound(0, it->index, it->value);
                    answer = check();
                    #endif
                    if (answer == Verifier::Answer::UNSAT) {
                        #ifdef MARABOU
                        it->value = tmp;
                        #endif
                        break;
                    }
                }

              if (answer != Verifier::Answer::UNSAT)
                  it->value = inputValue;
            }
            #ifndef MARABOU
            verifier->pop();
            verifier->push();
            #endif
        }
    }

    #ifndef MARABOU
    solver.pop();
    #endif

    GeneralizedExplanation generalizedExplanation;
    auto & constraints = generalizedExplanation.constraints;
    size_t fixedFeaturesCount = 0;
    for (auto const & lb : lowerBounds) {
        constraints.push_back({.inputIndex = lb.index, .boundType = GeneralizedExplanation::BoundType::LB, .value = lb.value});
    }
    for (auto const & ub : upperBounds) {
        auto it = std::find_if(constraints.begin(), constraints.end(), [&](auto const & bound){ return bound.inputIndex == ub.index; });
        if (it != constraints.end() and it->value == ub.value) {
            it->boundType = GeneralizedExplanation::BoundType::EQ;
            ++fixedFeaturesCount;
        } else {
            constraints.push_back({.inputIndex = ub.index, .boundType = GeneralizedExplanation::BoundType::UB, .value = ub.value});
        }
    }
    std::sort(constraints.begin(), constraints.end(), [](auto const & first, auto const & second) {
        return first.inputIndex < second.inputIndex or (first.inputIndex == second.inputIndex and first.boundType == GeneralizedExplanation::BoundType::LB and second.boundType != GeneralizedExplanation::BoundType::LB);
    });
    std::cout << "fixed features: " << fixedFeaturesCount << "/" << inputValues.size() << std::endl;
    std::cout << "size: " << explanationFeatures.size() << "/" << inputValues.size() << std::endl;
    return generalizedExplanation;
}

BasicVerix::Result BasicVerix::computeOpenSMTExplanation(input_t const & inputValues, float freedom_factor,
                                                         std::vector<std::size_t> const & featureOrder) {
    checksCount = 0;
    assert(freedom_factor <= 1.0 and freedom_factor >= 0.0);
    if (not network or not verifier)
        return Result{};

    verifier->init();
    verifier->loadModel(*network);
    if constexpr (Config::interpolation) {
        // to reproduce the same itps ...
        verifier->push();
    }

    auto output = nn::computeOutput(inputValues, *network);
    NodeIndex label = std::max_element(output.begin(), output.end()) - output.begin();
    auto inputSize = network->getLayerSize(0);

    // Compute lower and upper bounds based on the freedom factor
    std::vector<float> inputLowerBounds;
    std::vector<float> inputUpperBounds;
    for (NodeIndex node = 0; node < inputSize; ++node) {
        float input_freedom = (network->getInputUpperBound(node) - network->getInputLowerBound(node)) * freedom_factor;
        inputLowerBounds.push_back(std::max(inputValues[node] - input_freedom, network->getInputLowerBound(node)));
        inputUpperBounds.push_back(std::min(inputValues[node] + input_freedom, network->getInputUpperBound(node)));
    }

    assert(inputSize == inputValues.size());
    assert(featureOrder.size() == inputSize);
    for (NodeIndex node = 0; node < inputSize; ++node) {
        verifier->addLowerBound(0, node, inputLowerBounds[node]);
        verifier->addUpperBound(0, node, inputUpperBounds[node]);
    }
    encodeClassificationConstraint(output, label);

    assert(dynamic_cast<verifiers::OpenSMTVerifier *>(verifier.get()));
    auto & openSMT = static_cast<verifiers::OpenSMTVerifier &>(*verifier);
    auto & solver = openSMT.getSolver();

    // to get psi:
    // solver.printCurrentAssertionsAsQuery();

    if constexpr (Config::interpolation) {
        solver.push();
    }
    for (NodeIndex node : featureOrder) {
        auto const val = inputValues[node];
        assert(val >= inputLowerBounds[node]);
        assert(val <= inputUpperBounds[node]);

        if constexpr (Config::samplesOnly) {
            verifier->addEquality(0, node, val, true);
            continue;
        }

        bool const isLower = (val == inputLowerBounds[node]);
        bool const isUpper = (val == inputUpperBounds[node]);
        // It is worthless to assert bounds that already correspond to the bounds of the domain
        if (not isLower) { verifier->addLowerBound(0, node, val, true); }
        if (not isUpper) { verifier->addUpperBound(0, node, val, true); }
    }
    auto answer = check();
    assert(answer == Verifier::Answer::UNSAT);

    if constexpr (Config::interpolation) {
        auto const unsatCore = solver.getUnsatCore();

        solver.pop();

        auto const firstIdx = solver.getInsertedFormulasCount();
        for (PTRef term : unsatCore->getTerms()) {
            solver.insertFormula(term);
        }
        auto const lastIdx = solver.getInsertedFormulasCount()-1;

        auto answer = check();
        assert(answer == Verifier::Answer::UNSAT);

        ipartitions_t part = 0;
        for (auto idx = firstIdx; idx <= lastIdx; ++idx) {
            setbit(part, idx);
        }

        vec<PTRef> itps;
        auto interpolationContext = solver.getInterpolationContext();
        interpolationContext->getSingleInterpolant(itps, part);
        assert(itps.size() == 1);
        PTRef itp = itps[0];

        std::cerr << solver.getLogic().printTerm(itp) << std::endl << std::endl;

        return {};
    }

    assert(dynamic_cast<verifiers::UnsatCoreVerifier *>(verifier.get()));
    auto & ucoreVerifier = static_cast<verifiers::UnsatCoreVerifier &>(*verifier);

    verifiers::UnsatCore unsatCore = ucoreVerifier.getUnsatCore();

    // to get the whole query:
    // solver.printCurrentAssertionsAsQuery();

    {
        auto & logic = solver.getLogic();
        // !! build it for the second time
        PTRef ucore = logic.mkAnd(solver.getUnsatCore()->getTerms());
        std::cerr << logic.printTerm(ucore) << std::endl << std::endl;
    }

    if constexpr (Config::samplesOnly) {
        assert(unsatCore.lowerBounds.empty());
        assert(unsatCore.upperBounds.empty());
        std::vector<NodeIndex> explanation = std::move(unsatCore.equalities);
        assert(std::ranges::is_sorted(explanation));
        return Result{.explanation = std::move(explanation)};
    }

    assert(unsatCore.equalities.empty());

    std::unordered_set<NodeIndex> explanationLowerSet;
    std::unordered_set<NodeIndex> explanationUpperSet;
    for (NodeIndex node : unsatCore.lowerBounds) { explanationLowerSet.insert(node); }
    for (NodeIndex node : unsatCore.upperBounds) { explanationUpperSet.insert(node); }
    std::vector<NodeIndex> explanationLower = std::move(unsatCore.lowerBounds);
    std::vector<NodeIndex> explanationUpper = std::move(unsatCore.upperBounds);

    std::vector<NodeIndex> addExplanationLower;
    std::vector<NodeIndex> addExplanationUpper;
    double relVolume = 1;
    auto processCoreTerm = [&](auto isLower, NodeIndex node){
        auto & otherBoundExplanation = [&]() -> auto & {
            if constexpr (isLower) { return addExplanationUpper; }
            else { return addExplanationLower; }
        }();
        auto & otherBoundExplanationSet = [&]() -> auto & {
            if constexpr (isLower) { return explanationUpperSet; }
            else { return explanationLowerSet; }
        }();

        auto const val = inputValues[node];
        auto const lo = inputLowerBounds[node];
        auto const hi = inputUpperBounds[node];
        double const size = hi - lo;
        double const boundedSize = [=]{
            if constexpr (isLower) { return hi - val; }
            else { return val - lo; }
        }();
        assert(boundedSize >= 0);
        assert(boundedSize <= size);
        assert(boundedSize != size or (not isLower and otherBoundExplanationSet.contains(node)));

        if (otherBoundExplanationSet.contains(node)) {
            return;
        }

        if (boundedSize == 0) {
            otherBoundExplanation.push_back(node);
            return;
        }

        relVolume *= boundedSize/size;
    };
    std::ranges::for_each(explanationLower, [&](NodeIndex node){ processCoreTerm(std::true_type(), node); });
    std::ranges::for_each(explanationUpper, [&](NodeIndex node){ processCoreTerm(std::false_type(), node); });

    assert(std::ranges::is_sorted(explanationLower));
    assert(std::ranges::is_sorted(explanationUpper));
    assert(std::ranges::is_sorted(addExplanationLower));
    assert(std::ranges::is_sorted(addExplanationUpper));
    std::vector<NodeIndex> auxExplanationLower;
    std::vector<NodeIndex> auxExplanationUpper;
    std::ranges::merge(explanationLower, addExplanationLower, std::back_inserter(auxExplanationLower));
    std::ranges::merge(explanationUpper, addExplanationUpper, std::back_inserter(auxExplanationUpper));
    assert(std::ranges::is_sorted(auxExplanationLower));
    assert(std::ranges::is_sorted(auxExplanationUpper));
    explanationLower = std::move(auxExplanationLower);
    explanationUpper = std::move(auxExplanationUpper);

    std::vector<NodeIndex> explanationEqual;
    std::ranges::set_intersection(explanationLower, explanationUpper, std::back_inserter(explanationEqual));
    std::vector<NodeIndex> explanation;
    std::ranges::set_union(explanationLower, explanationUpper, std::back_inserter(explanation));

    auto const defaultPrecision = std::cout.precision();
    std::cout << "fixed features: " << explanationEqual.size() << "/" << inputSize << std::endl;
    std::cout << "relVolume*: " << std::setprecision(1) << (relVolume*100) << "%" << std::setprecision(defaultPrecision) << std::endl;
    return Result{.explanation = std::move(explanation)};
}

} // namespace xai::algo
