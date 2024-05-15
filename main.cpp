#include <iostream>
#include "NNModel.h"
#include "Verix.h"
//
//#include <stdio.h>


int main()
{
//    NNModel nn = NNModel();
    std::string filename = "models/heartAttack.nnet";
    // create randon vector of size 13
    std::vector<float> input_example = {};
    for(int i=0; i<13; i++){
        // push a random float between 0 and 1
        input_example.push_back((float)rand() / RAND_MAX);
    }
    std::vector<float> output_example = {0.0};



    Verix verix = Verix(filename, input_example, output_example);
    verix.get_explanation(2);
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

