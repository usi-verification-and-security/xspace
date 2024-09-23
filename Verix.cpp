//
// Created by labbaf on 26.04.2024.
//

#include <Opensmt.h>
#include "Verix.h"
#include <algorithm>

#include <fstream>
#include <vector>
#include <sstream>
#include <filesystem>

using namespace opensmt;

// Function to split a string by a delimiter
std::vector<std::string> split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void Verix::readNNetAndCreateFormulas(const std::string& filename, ArithLogic& logic, std::vector<PTRef>& inputVarsRefs, std::vector<PTRef>& outputVarsRefs) {
    /*
     * Read a .nnet file and create SMT formulas for the neural network.
     * param filename: the path to the .nnet file.
     * param logic: the logic.
     * param inputVarsRefs: the input variables.
     * param outputVarsRefs: the output variables.

     * Read the .nnet file and create SMT formulas for the neural network.
     * The file begins with header lines, some information about the network architecture, normalization information, and then model parameters. Line by line:
        1: Header text. This can be any number of lines so long as they begin with "//"
        2: Four values: Number of layers, number of inputs, number of outputs, and maximum layer size
        3: A sequence of values describing the network layer sizes. Begin with the input size, then the size of the first layer, second layer, and so on until the output layer size
        4: A flag that is no longer used, can be ignored
        5: Minimum values of inputs (used to keep inputs within expected range)
        6: Maximum values of inputs (used to keep inputs within expected range)
        7: Mean values of inputs and one value for all outputs (used for normalization)
        8: Range values of inputs and one value for all outputs (used for normalization)
        9+: Begin defining the weight matrix for the first layer, followed by the bias vector. The weights and biases for the second layer follow after, until the weights and biases for the output layer are defined.
     */

    if (!std::filesystem::exists(filename)) {
        std::cerr << "File does not exist: " << filename << std::endl;
        return;
    }
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    std::string line;

    // Skip header lines
    while (std::getline(file, line)) {
        if (line.find("//") == std::string::npos) {
            break;
        }
    }

    std::vector<std::string> networkArchitecture = split(line, ',');
    int numLayers = std::stoi(networkArchitecture[0]);
    int numInputs = std::stoi(networkArchitecture[1]);
    int numOutputs = std::stoi(networkArchitecture[2]);
    int maxLayerSize = std::stoi(networkArchitecture[3]);

    // Specify layer sizes
    std::getline(file, line);
    std::vector<std::string> layer_sizes = split(line, ',');

    std::getline(file, line);
    std::getline(file, line);
    std::vector<std::string> inputMinStr =  split(line, ',');
    for (const auto& inputMinStrElement : inputMinStr) {
        inputMins.push_back(std::stof(inputMinStrElement));
    }

    std::getline(file, line);
    std::vector<std::string> inputMaxStr =  split(line, ',');
    for (const auto& inputMaxStrElement : inputMaxStr) {
        inputMaxs.push_back(std::stof(inputMaxStrElement));
    }

    // Skip normalization information
    for (int i = 0; i < 2; i++) {
        std::getline(file, line);
    }

    // Parse model parameters
    std::vector<std::vector<std::vector<float>>> weights(numLayers);
    std::vector<std::vector<float>> biases(numLayers);
    for (int layer = 0; layer < numLayers; layer++) {
        // Parse weights
        for (int i = 0; i <  stoi(layer_sizes[layer+1]); i++) {
            std::getline(file, line);
            std::vector<std::string> weightStrings = split(line, ',');
            weights[layer].push_back(std::vector<float>());
            for (const auto& weightString : weightStrings) {
                weights[layer][i].push_back(std::stof(weightString));
            }
        }

        // Parse biases
        for (int i = 0; i <  stoi(layer_sizes[layer+1]); i++) {
            std::getline(file, line);
            std::vector<std::string> biasStrings = split(line, ',');
            std::string biasString = biasStrings[0];
            biases[layer].push_back(std::stof(biasString));
        }
    }
    file.close();

    // define input variables
    for (int i = 0; i < numInputs; i++) {
        std::string name = "input_" + std::to_string(i);
        inputVarsRefs.push_back(logic.mkRealVar(name.c_str()));
    }

    // Create SMT formulas
    std::vector<PTRef> previousLayerRefs = inputVarsRefs;
    int denominator = 10000000; // Adjust this value as needed
    PTRef zero = logic.mkRealConst(FastRational(0));
    for (int layer = 0; layer < numLayers; layer++) {
        std::vector<PTRef> currentLayerRefs;
        for (int i = 0; i < biases[layer].size(); i++) {
            int numerator_biase = static_cast<int>(biases[layer][i] * denominator);
            PTRef sum;
            sum = logic.mkRealConst(FastRational(numerator_biase, denominator));

            for (int j = 0; j < weights[layer][i].size(); j++) {
                int numerator_weight = static_cast<int>(weights[layer][i][j] * denominator);
                sum = logic.mkPlus(sum, logic.mkTimes(logic.mkRealConst(FastRational(numerator_weight, denominator)), previousLayerRefs[j]));
            }
            PTRef relu = logic.mkIte(logic.mkGt(sum, zero), sum, zero);
            currentLayerRefs.push_back(relu);
        }
        previousLayerRefs = currentLayerRefs;
    }
    outputVarsRefs = previousLayerRefs;
}

Verix::Verix(const std::string model_file, std::vector<float> inputVals, std::vector<float> outputVals)
    : model_file(model_file),
      input_examples(inputVals),
      output_examples(outputVals)
{
}

void Verix::get_explanation(float freedom_factor) {
    /*
     * To compute the explanation for the model and the neural network.
     * param freedom_factor: the perturbation magnitude.
     * return: an explanation, and possible counterfactual(s).
     */

    Opensmt* osmt = pre();
    SMTConfig& c = osmt->getConfig();
    MainSolver& mainSolver = osmt->getMainSolver();
    auto & logic = osmt->getLRALogic();


    std::vector<int> unsat_set;
    std::vector<int> sat_set;
    std::vector<int> timeout_set;

    std::vector<PTRef> outputVarsRefs;
    std::vector<PTRef> inputVarsRefs;

    /*
     * add model encoding and create the outputs
     * */
    readNNetAndCreateFormulas(model_file, logic, inputVarsRefs, outputVarsRefs);
    std::vector<float> inputLowerBounds;
    std::vector<float> inputUpperBounds;
    for(int i = 0; i < input_examples.size(); i++){
        float  input_freedom = (inputMaxs[i] - inputMins[i]) * freedom_factor;
        inputLowerBounds.push_back(std::max(input_examples[i] - input_freedom, inputMins[i]));
        inputUpperBounds.push_back(std::min(input_examples[i] + input_freedom, inputMaxs[i]));
    }

    //set bounds on input variables (based on the training data)
    for (int i = 0; i < input_examples.size(); i++) {
            FastRational lower_bound_fr(inputMins[i], 1); // Create FastRational from float
            FastRational upper_bound_fr(inputMins[i], 1); // Create FastRational from float
            mainSolver.insertFormula(logic.mkAnd(logic.mkLeq(inputVarsRefs[i], logic.mkRealConst(upper_bound_fr)),
                                                 logic.mkLeq(logic.mkRealConst(lower_bound_fr), inputVarsRefs[i])));
    }
    // add constraints on the output variable
    if(output_examples.size() == 1){
        if(output_examples[0] > 0.5){
            mainSolver.insertFormula(logic.mkLeq(outputVarsRefs[0], logic.mkRealConst(FastRational(1,2))));
        } else {
            mainSolver.insertFormula(logic.mkGeq(outputVarsRefs[0], logic.mkRealConst(FastRational(1, 2))));
        }
    } else {
//        TODO: add constraints for multi-class classification
    }

    //only if you want to freeze all the inputs
//    for (int i = 0; i < input_examples.size(); i++) {
////                set lower bound for the variable
//            FastRational lower_bound_fr(input_examples[i], 1); // Create FastRational from float
////                set upper bound for the variable
//            FastRational upper_bound_fr(input_examples[i], 1); // Create FastRational from float
//            mainSolver.insertFormula(logic.mkAnd(logic.mkLeq(inputVarsRefs[i], logic.mkRealConst(upper_bound_fr)),
//                                                 logic.mkLeq(logic.mkRealConst(lower_bound_fr), inputVarsRefs[i])));
//    }
//    auto res = mainSolver.check();
//    if (res == s_True) {
//        printf("sat\n");
//        auto m = mainSolver.getModel();
//        std::vector<float> cex;
//        for (int i = 0; i < inputVarsRefs.size(); i++) {
//            auto v1_p = logic.pp(m->evaluate(inputVarsRefs[i]));
//            auto var_a_s = logic.pp(inputVarsRefs[i]);
//            printf("%s, ", v1_p.c_str());
//        }
//        printf("\n");
//
//    } else if (res == s_False) {
//        printf("unsat\n");
//        mainSolver.printFramesAsQuery();
//        auto itp = mainSolver.getInterpolationContext();
//        vec<PTRef> itps;
//        ipartitions_t partitions = 1;
//        itp->getSingleInterpolant(itps, partitions);
//        assert(itps.size() == 1);
//        PTRef itp_formula = itps[0];
//        std::cout << logic.pp(itps[0]);
//        logic.getPterm(itp_formula);
//    } else if (res == s_Undef) {
//        printf("unknown\n");
//    } else {
//        printf("error\n");
//    }
    int label;
    if (output_examples.size() == 1){
        label = output_examples[0] > 0.5 ? 1 : 0;
    }else {
        //TODO: These two lines are suspecoius
        auto maxElementIter = std::max_element(output_examples.begin(), output_examples.end());
        label = std::distance(output_examples.begin(), maxElementIter);
    }
    float lower_bound;
    float upper_bound;
    for (int feature = 0; feature < input_examples.size(); feature++) {
        mainSolver.push();
        for (int i = 0; i < input_examples.size(); i++) {
            /*
             * Set constraints on the input variables.
             */
            if (i == feature || std::find(unsat_set.begin(), unsat_set.end(), i) != unsat_set.end()) {
                /*
                 * Set allowable perturbations on the current feature and the irrelevant features.
                 */
                lower_bound =  inputLowerBounds[i];
                upper_bound =  inputUpperBounds[i];
//                set lower bound for the variable
                    FastRational lower_bound_fr(lower_bound, 1); // Create FastRational from float
                mainSolver.insertFormula(logic.mkLeq( logic.mkRealConst(lower_bound_fr), inputVarsRefs[i]));
//                set upper bound for the variable
                FastRational upper_bound_fr(upper_bound, 1); // Create FastRational from float
                mainSolver.insertFormula(logic.mkLeq(inputVarsRefs[i], logic.mkRealConst(upper_bound_fr)));
            } else {
                /*
                 * Make sure the other pixels are fixed.
                 */
                lower_bound =  input_examples[i];
                upper_bound =  input_examples[i];
//                set lower bound for the variable
                FastRational lower_bound_fr(lower_bound, 1); // Create FastRational from float
                mainSolver.insertFormula(logic.mkLeq(inputVarsRefs[i], logic.mkRealConst(lower_bound_fr)));
//                set upper bound for the variable
                FastRational upper_bound_fr(upper_bound, 1); // Create FastRational from float
                mainSolver.insertFormula(logic.mkLeq(logic.mkRealConst(upper_bound_fr), inputVarsRefs[i]));
            }
        }

//        TODO: uncomment for multi-class classification
//        for (int j = 0; j < output_examples.size(); j++) {
//            /*
//             * Set constraints on the output variables.
//             */
//            if (j != label) {
//                // [1, -1], -1e-6,
//                // isProperty=True)
//                vec<PTRef> args_lt;
//                args_lt.push(outputVarsRefs[label]);
//                args_lt.push(outputVarsRefs[j]);
//                PTRef formula = logic.mkLt(args_lt);
//                mainSolver.insertFormula(formula);
//            }
//        }

        sstat r = mainSolver.check();
        if (r == s_True) {
            printf("sat\n");
            sat_set.push_back(feature);
            auto m = mainSolver.getModel();
            std::vector<float> cex;
            for (int i = 0; i < inputVarsRefs.size(); i++) {
                auto v1_p = logic.pp(m->evaluate(inputVarsRefs[i]));
                auto var_a_s = logic.pp(inputVarsRefs[i]);
                printf("%s, ", v1_p.c_str());
            }
            printf("\n");
//            const std::vector<float> &prediction = nn.predict(input_examples);


        } else if (r == s_False) {
            printf("unsat\n");
            mainSolver.printFramesAsQuery();
            auto itp = mainSolver.getInterpolationContext();
            vec<PTRef> itps;
            ipartitions_t partitions = 1;
            itp->getSingleInterpolant(itps, partitions);
            assert(itps.size() == 1);
            std::cout << logic.pp(itps[0]);
            unsat_set.push_back(feature);
        } else if (r == s_Undef) {
            printf("unknown\n");
            timeout_set.push_back(feature);
        } else {
            printf("error\n");
        }
        for (int i = 0; i < input_examples.size(); i++) {
            mainSolver.pop();
        }
//        mainSolver.pop();
    }

//    print sat_set
    printf("Explanation:  \n");
    for (int i = 0; i < sat_set.size(); i++) {
        printf("%d ", sat_set[i]);
    }
    printf("\n");
}
