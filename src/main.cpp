#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <algorithms/BasicVerix.h>
#include <experiments/VerixExperiments.h>

using namespace xai;

int main(int argc, char* argv[])
{
    #ifndef MARABOU
    std::string verifier = "OpenSMT";
    #else
    std::string verifier = "Marabou";
    #endif
    #ifndef OBESITY
    std::string filename = "models/Heart_attack/heartAttack50.nnet";
    std::string datafile = "data/heartAttack.csv";
    #else
    std::string filename = "models/obesity/obesity-10-20-10.nnet";
    std::string datafile = "data/obesity_test_data_noWeight.csv";
    #endif
    float freedom_factor = 1;

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << freedom_factor;
    std::string freedom_factor_str = stream.str();
    std::string outputfile = "output/HA50_" + verifier + "_" + freedom_factor_str + ".csv";

    if (argc > 1) {
        filename = argv[1];
    }
    #ifndef OBESITY
    experiments::VerixExperiments::experiment_on_dataset(filename, datafile, verifier, outputfile, 13, freedom_factor);
    #else
    experiments::VerixExperiments::experiment_on_dataset(filename, datafile, verifier, outputfile, 15, freedom_factor);
    #endif
}
