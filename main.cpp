#include <iostream>
#include "NNModel.h"
#include "Verix.h"
#include "algorithms/BasicVerix.h"
#include "verifiers/opensmt/OpenSMTVerifier.h"
#include "verifiers/marabou/MarabouVerifier.h"
#include "experiments/VerixExperiments.h"
#include <iomanip>
//#include <stdio.h>


int main(int argc, char* argv[])
{
    std::string verifier = "OpenSMT";
    std::string filename = "models/heartAttack.nnet";
    std::string datafile = "data/heartAttack.csv";
    float freedom_factor = 0.5;

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << freedom_factor;
    std::string freedom_factor_str = stream.str();
    std::string outputfile = "output/HA_" + verifier + "_" + freedom_factor_str + ".csv";

    if (argc > 1) {
        filename = argv[1];
    }
//    xai::algo::BasicVerix algo(filename);
//    algo.setVerifier(std::make_unique<xai::verifiers::MarabouVerifier>());


//    std::vector<float> input_example = {63, 1, 3, 145, 233, 1, 0, 150, 0, 2.3, 0, 0, 1};
//    auto res = algo.computeExplanation({1,0,0}, 0.5);
    VerixExperiments::experiment_on_dataset(filename, datafile, verifier, outputfile, 13, freedom_factor);

    exit(0);


//    Verix verix = Verix(filename, input_example, output_example);
//    verix.get_explanation(1);
//    Logic logic{opensmt::Logic_t::QF_UF}; // UF Logic
//    SMTConfig c;
//    MainSolver mainSolver(logic, c, "test solver");
//    PTRef v = logic.mkBoolVar("a");
//    PTRef v_neg = logic.mkNot(v);
//    vec<PTRef> args;
//    args.push(v);
//    args.push(v_neg);
//    PTRef a = logic.mkAnd(args);
//
//    mainSolver.insertFormula(a);
//    printf("Running check!\n");
//    sstat r = mainSolver.check();
//
//    if (r == s_True)
//        printf("sat\n");
//    else if (r == s_False)
//        printf("unsat\n");
//    else if (r == s_Undef)
//        printf("unknown\n");
//    else
//        printf("error\n");

//    printf("Done without any serious errors\n");

//    return 0;
}

