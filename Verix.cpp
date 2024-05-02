//
// Created by labbaf on 26.04.2024.
//

#include <Opensmt.h>
#include "Verix.h"
#include <algorithm>



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
     * param plot_explanation: if True, plot the explanation.
     * param plot_counterfactual: if True, plot the counterfactual(s).
     * param plot_timeout: if True, plot the timeout pixel(s).
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
//        name = "input_" + std::to_string(i);
        std::string name = "input_" + std::to_string(i);
        inputVarsRefs.push_back(logic.mkRealVar(name.c_str()));
    }


//    for (int i = 0; i < outputVars.size(); i++) {
//        outputVarsRefs.push_back(logic.mkRealVar(inputVars[i].c_str()));
//    }

    std::vector<float> output_vars = nn.predict(input_example);
    auto maxElementIter = std::max_element(output_vars.begin(), output_vars.end());
    int label = std::distance(output_vars.begin(), maxElementIter);

    float lower_bound;
    float upper_bound;
    for (int feature = 0; feature < input_example.size(); feature++) {
        for (int i = 0; i < input_example.size(); i++) {
            /*
             * Set constraints on the input variables.
             */

            if (i == feature || std::find(unsat_set.begin(), unsat_set.end(), i) != unsat_set.end()) {
                /*
                 * Set allowable perturbations on the current feature and the irrelevant features.
                 */
                lower_bound =  input_example[i] - epsilon;
                upper_bound =  input_example[i] + epsilon;
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

        /*
         * add model encoding and create the outputs
         * TODO: you should do this automatically in the model class
         * */
        // Create constants
        PTRef five = logic.mkRealConst(FastRational(5));
        PTRef minus_one = logic.mkRealConst(FastRational(-1));
        PTRef minus_half = logic.mkRealConst(FastRational(-1, 2));;
        PTRef zero = logic.mkRealConst(FastRational(0));

        // Create formulas
        PTRef h1 = logic.mkMinus(logic.mkTimes(five, inputVarsRefs[0]), logic.mkPlus(inputVarsRefs[1], inputVarsRefs[2]));
        PTRef h2 = logic.mkPlus(logic.mkMinus(inputVarsRefs[1], inputVarsRefs[0]), inputVarsRefs[2]);

        // Create ReLU function
        PTRef h1_relu = logic.mkIte(logic.mkGt(h1, zero), h1, zero);
        PTRef h2_relu = logic.mkIte(logic.mkGt(h2, zero), h2, zero);

        // Create output formulas
        PTRef o1 = logic.mkMinus(h1_relu, h2_relu);
        PTRef o2 = logic.mkPlus(logic.mkTimes(minus_half, h1_relu), h2_relu);
        outputVarsRefs.push_back(o1);
        outputVarsRefs.push_back(o2);


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
//            print the prediction
            for (int i = 0; i < prediction.size(); i++) {
                printf("%f ", prediction[i]);
            }
            printf("\n");


        } else if (r == s_False) {
            printf("unsat\n");
            unsat_set.push_back(feature);
        } else if (r == s_Undef) {
            printf("unknown\n");
            timeout_set.push_back(feature);
        } else {
            printf("error\n");
        }
    }

//    print sat_set
    printf("Explanation:  \n");
    for (int i = 0; i < sat_set.size(); i++) {
        printf("%d ", sat_set[i]);
    }
    printf("\n");
}