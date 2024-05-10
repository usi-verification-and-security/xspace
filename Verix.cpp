//
// Created by labbaf on 26.04.2024.
//

#include <Opensmt.h>
#include "Verix.h"
#include <algorithm>

#include <fstream>
#include <vector>
#include <sstream>

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

void readNNetAndCreateFormulas(const std::string& filename, ArithLogic& logic, std::vector<PTRef>& inputVarsRefs, std::vector<PTRef>& outputVarsRefs) {
    /*
     * Read a .nnet file and create SMT formulas for the neural network.
     * param filename: the path to the .nnet file.
     * param logic: the logic.
     * param inputVarsRefs: the input variables.
     * param outputVarsRefs: the output variables.
     */

    /*
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

    // Skip normalization information
    for (int i = 0; i < 6; i++) {
        std::getline(file, line);
    }

    // Parse model parameters
    std::vector<std::vector<std::vector<float>>> weights(numLayers);
    std::vector<std::vector<float>> biases(numLayers);
    for (int layer = 0; layer < numLayers; layer++) {
        // Parse weights
        for (int i = 0; i < (layer == 0 ? numInputs : weights[layer - 1].size()); i++) {
            std::getline(file, line);
            std::vector<std::string> weightStrings = split(line, ',');
            weights[layer].push_back(std::vector<float>());
            for (const auto& weightString : weightStrings) {
                weights[layer][i].push_back(std::stof(weightString));
            }
        }

        // Parse biases
        std::getline(file, line);
        std::vector<std::string> biasStrings = split(line, ',');
        for (const auto& biasString : biasStrings) {
            biases[layer].push_back(std::stof(biasString));
        }
    }

    // Create SMT formulas
    std::vector<PTRef> previousLayerRefs = inputVarsRefs;
    int denominator = 1000000000; // Adjust this value as needed
    PTRef zero = logic.mkRealConst(FastRational(0));
    for (int layer = 0; layer < numLayers; layer++) {
        std::vector<PTRef> currentLayerRefs;
        for (int i = 0; i < weights[layer].size(); i++) {
            int numerator_biases = static_cast<int>(biases[layer][i] * denominator);
            PTRef sum;
            if (numerator_biases == 0) {
                sum = logic.mkRealConst(FastRational(numerator_biases));
            } else {
                sum = logic.mkRealConst(FastRational(numerator_biases, denominator));
            }

            for (int j = 0; j < weights[layer][i].size(); j++) {
                int numerator_weights = static_cast<int>(weights[layer][i][j] * denominator);
                sum = logic.mkPlus(sum, logic.mkTimes(logic.mkRealConst(FastRational(numerator_weights, denominator)), previousLayerRefs[j]));
            }
            PTRef relu = logic.mkIte(logic.mkGt(sum, zero), sum, zero);
            currentLayerRefs.push_back(relu);
        }
        previousLayerRefs = currentLayerRefs;
    }
    outputVarsRefs = previousLayerRefs;
}

Verix::Verix(const NNModel& model)
    : nn(model),
      inputVars({"x1", "x2", "x3"}),
      outputVars({"o1", "o2"})
{

    // this->nn = model;
    // this->logic = opensmt::Logic_t::QF_UF;
    // this->mainSolver = MainSolver(logic, SMTConfig(), "Verix solver");
}

void Verix::get_explanation(float epsilon, std::vector<float> input_example) {
    /*
     * To compute the explanation for the model and the neural network.
     * param epsilon: the perturbation magnitude.
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
    for (int i = 0; i < inputVars.size(); i++) {
        std::string name = "input_" + std::to_string(i);
        inputVarsRefs.push_back(logic.mkRealVar(name.c_str()));
    }

    /*
     * add model encoding and create the outputs
     * */
    std::string filename = "models/dummy_network.nnet";
    readNNetAndCreateFormulas(filename, logic, inputVarsRefs, outputVarsRefs);

//    {
//        for (int i = 0; i < input_example.size(); ++i) {
////                set lower bound for the variable
//                FastRational lower_bound_fr(input_example[i], 1); // Create FastRational from float
////                set upper bound for the variable
//                FastRational upper_bound_fr(input_example[i], 1); // Create FastRational from float
//                mainSolver.insertFormula(logic.mkAnd(logic.mkLeq(inputVarsRefs[i], logic.mkRealConst(lower_bound_fr)),
//                                                     logic.mkLeq(logic.mkRealConst(upper_bound_fr), inputVarsRefs[i])));
//        }
//        mainSolver.insertFormula(logic.mkLt(outputVarsRefs[0], outputVarsRefs[1]));
//        auto res = mainSolver.check();
//        assert(res == s_False);
//        auto itp = mainSolver.getInterpolationContext();
//        vec<PTRef> itps;
//        ipartitions_t partitions = 1;
//        itp->getSingleInterpolant(itps, partitions);
//        assert(itps.size() == 1);
//        std::cout << logic.pp(itps[0]);
//        exit(1);
//    }

    std::vector<float> output_vars = nn.predict(input_example);
    auto maxElementIter = std::max_element(output_vars.begin(), output_vars.end());
    int label = std::distance(output_vars.begin(), maxElementIter);

    float lower_bound;
    float upper_bound;
    for (int feature = 0; feature < input_example.size(); feature++) {
        mainSolver.push();
        for (int i = 0; i < input_example.size(); i++) {
            /*
             * Set constraints on the input variables.
             */

            if (i == feature || std::find(unsat_set.begin(), unsat_set.end(), i) != unsat_set.end()) {
                /*
                 * Set allowable perturbations on the current feature and the irrelevant features.
                 */
                lower_bound =  std::max(input_example[i] - epsilon, 0.0f);
                upper_bound =  std::min(input_example[i] + epsilon, 1.0f);
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
                lower_bound =  input_example[i];
                upper_bound =  input_example[i];
//                set lower bound for the variable
                FastRational lower_bound_fr(lower_bound, 1); // Create FastRational from float
                mainSolver.insertFormula(logic.mkLeq(inputVarsRefs[i], logic.mkRealConst(lower_bound_fr)));
//                set upper bound for the variable
                FastRational upper_bound_fr(upper_bound, 1); // Create FastRational from float
                mainSolver.insertFormula(logic.mkLeq(logic.mkRealConst(upper_bound_fr), inputVarsRefs[i]));
            }
        }


        for (int j = 0; j < output_vars.size(); j++) {
            /*
             * Set constraints on the output variables.
             */
            if (j != label) {
                // [1, -1], -1e-6,
                // isProperty=True)
                vec<PTRef> args_lt;
                args_lt.push(outputVarsRefs[label]);
                args_lt.push(outputVarsRefs[j]);
                PTRef formula = logic.mkLt(args_lt);
                mainSolver.insertFormula(formula);

            }
        }

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
            const std::vector<float> &prediction = nn.predict(input_example);


        } else if (r == s_False) {
            printf("unsat\n");
            unsat_set.push_back(feature);
        } else if (r == s_Undef) {
            printf("unknown\n");
            timeout_set.push_back(feature);
        } else {
            printf("error\n");
        }
        mainSolver.pop();
    }

//    print sat_set
    printf("Explanation:  \n");
    for (int i = 0; i < sat_set.size(); i++) {
        printf("%d ", sat_set[i]);
    }
    printf("\n");
}