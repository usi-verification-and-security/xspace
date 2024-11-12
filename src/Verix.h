//
// Created by labbaf on 26.04.2024.
//

#ifndef XAI_SMT_VERIX_H
#define XAI_SMT_VERIX_H


#include "NNModel.h"
#include <api/Opensmt.h>
#include <vector>
#include "Verifier.h"

class Verix {
    private:
    std::string model_file;
//    Logic logic;
//    SMTConfig config;
//    MainSolver mainSolver;
    std::vector<float> input_examples;
    std::vector<float> output_examples;
    std::vector<float> inputMaxs;
    std::vector<float> inputMins;
    ;

    public:
    Verix(std::string model_file, std::vector<float> inputVals, std::vector<float> outputVals);
    void get_explanation(float get_explanation);

    opensmt::Opensmt*
    pre()
    {
        using namespace opensmt;

        auto config = std::make_unique<SMTConfig>();
        const char* msg;
        config->setOption(SMTConfig::o_produce_inter, SMTOption(true), msg);
        Opensmt* osmt = new Opensmt(opensmt_logic::qf_lra, "test solver", std::move(config));
        return osmt;
    }

    void readNNetAndCreateFormulas(const std::string &filename, opensmt::ArithLogic &logic, std::vector<opensmt::PTRef> &inputVarsRefs,
                                   std::vector<opensmt::PTRef> &outputVarsRefs);
};
//
//def get_explanation(self, epsilon):
//    """
//    To compute the explanation for the model and the neural network.
//    :param epsilon: the perturbation magnitude.
//    :param plot_explanation: if True, plot the explanation.
//    :param plot_counterfactual: if True, plot the counterfactual(s).
//    :param plot_timeout: if True, plot the timeout pixel(s).
//    :return: an explanation, and possible counterfactual(s).
//    """
//    unsat_set = []
//    sat_set = []
//    timeout_set = []
//    input_example = self.input_example
//    for feature in self.inputVars:
//    for i in self.inputVars:
//            """
//            Set constraints on the input variables.
//            """
//            if i == feature or i in unsat_set:
//            """
//            Set allowable perturbations on the current feature and the irrelevant features.
//            """
//            self.mara_model.setLowerBound(i, max(0, input_example[i] - epsilon))
//            self.mara_model.setUpperBound(i, min(1, input_example[i] + epsilon))
//
//        else:
//        """
//        Make sure the other pixels are fixed.
//        """
//        self.mara_model.setLowerBound(i, input_example[i])
//        self.mara_model.setUpperBound(i, input_example[i])
//
//        for j in range(len(self.outputVars)):
//        """
//        Set constraints on the output variables.
//    """
//    if j != self.label:
//    self.mara_model.addInequality([self.outputVars[self.label], self.outputVars[j]],
//    [1, -1], -1e-6,
//    isProperty=True)
//    exit_code, vals, stats = self.mara_model.solve(verbose=True)
//    """
//    additionalEquList.clear() is to clear the output constraints.
//    """
//    self.mara_model.additionalEquList.clear()
//    if exit_code == 'sat' or exit_code == 'TIMEOUT':
//    break
//    elif exit_code == 'unsat':
//    continue
//    """
//    clearProperty() is to clear both input and output constraints.
//    """
//    self.mara_model.clearProperty()
//    """
//    If unsat, put the pixel into the irrelevant set;
//    if timeout, into the timeout set;
//    if sat, into the explanation.
//    """
//    if exit_code == 'unsat':
//    unsat_set.append(feature)
//    elif exit_code == 'TIMEOUT':
//    timeout_set.append(feature)
//    elif exit_code == 'sat':
//    sat_set.append(feature)
//
//    counterfactual = [vals.get(i) for i in self.mara_model.inputVars[0].flatten()]
//    output = [vals.get(i) for i in self.outputVars]
//    prediction = np.asarray(output).argmax()
//    print("Counterfactual for input ", feature, " : ", counterfactual)
//    print("Output: ", output)
//    print("Prediction: ", prediction)
//
//    explanation = [self.input_example[i] for i in sat_set]
//    print("Explanation: ", sat_set)
//
//
//
#endif //XAI_SMT_VERIX_H
