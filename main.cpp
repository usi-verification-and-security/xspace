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
    std::string filename = "models/heartAttack50.nnet";
    std::string datafile = "data/heartAttack.csv";
    float freedom_factor = 1;

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << freedom_factor;
    std::string freedom_factor_str = stream.str();
    std::string outputfile = "output/HA50_" + verifier + "_" + freedom_factor_str + ".csv";

    if (argc > 1) {
        filename = argv[1];
    }
    VerixExperiments::experiment_on_dataset(filename, datafile, verifier, outputfile, 13, freedom_factor);
}

