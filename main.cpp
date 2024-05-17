#include <iostream>
#include "NNModel.h"
#include "Verix.h"
#include "algorithms/BasicVerix.h"
#include "verifiers/opensmt/OpenSMTVerifier.h"
#include "verifiers/marabou/MarabouVerifier.h"
//
//#include <stdio.h>


int main(int argc, char* argv[])
{
    std::string filename = "models/heartAttack.nnet";
//    std::string filename = "models/dummy_network.nnet";
//    std::string filename = "models/test_network.nnet";
    if (argc > 1) {
        filename = argv[1];
    }
    xai::algo::BasicVerix algo(filename);
    algo.setVerifier(std::make_unique<xai::verifiers::MarabouVerifier>());
//    algo.setVerifier(std::make_unique<xai::verifiers::OpenSMTVerifier>());
//    algo.computeExplanation({1,0,0}, 0.1);
    auto res = algo.computeExplanation({63, 1, 3, 145, 233, 1, 0, 150, 0, 2.3, 0, 0, 1}, 1);
//    auto res = algo.computeExplanation({1,1}, 1);
    for (auto val : res.explanation) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    exit(0);


    std::vector<float> input_example = {63, 1, 3, 145, 233, 1, 0, 150, 0, 2.3, 0, 0, 1};
//    for(int i=0; i<13; i++){
//        // push a random float between 0 and 1
//        input_example.push_back((float)rand() / RAND_MAX);
//    }
    std::vector<float> output_example = {1.0};



    Verix verix = Verix(filename, input_example, output_example);
    verix.get_explanation(1);
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

    printf("Done without any serious errors\n");

    return 0;
}

