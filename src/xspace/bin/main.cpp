#include <xspace/explanation/Explanation.h>
#include <xspace/framework/Config.h>
#include <xspace/framework/Framework.h>
#include <xspace/nn/Dataset.h>

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include <getopt.h>

namespace {
void printUsageStrategyRow(std::ostream & os, std::string_view name, std::vector<std::string_view> params = {}) {
    os << "    " << name;
    if (not params.empty()) {
        os << "\tPARAMS:";
        for (auto param : params) {
            os << ' ' << param;
        }
    }
    os << '\n';
}

void printUsageOptRow(std::ostream & os, char opt, std::string_view desc) {
    os << "    -" << opt << "      " << desc << '\n';
}

void printUsage(std::ostream & os = std::cout) {
    os << "USAGE: xspace <nn_model_fn> <dataset_fn> <verifier_name> <exp_strategies_spec> [<options>]\n";
    os << "VERIFIERS: opensmt\n";
    os << "STRATEGIES SPEC: '<spec1>[,<spec2>]...'\n";
    os << "Each spec: '<name>[ <param>]...'\n";
    printUsageStrategyRow(os, "abductive");
    printUsageStrategyRow(os, "ucore", {"sample", "interval"});
    os << "OPTIONS:\n";
    printUsageOptRow(os, 'h', "Prints this help message and exits");
    printUsageOptRow(os, 'v', "Run in verbose mode");
    printUsageOptRow(os, 'r', "Reverse the order of variables");
    os.flush();
}
} // namespace

int main(int argc, char * argv[]) try {
    constexpr int minArgs = 4;

    int const nArgs = argc - 1;
    assert(nArgs >= 0);
    if (nArgs == 0) {
        printUsage();
        return 0;
    }

    if (nArgs < minArgs) {
        std::cerr << "Expected at least " << minArgs << " arguments, got: " << nArgs << '\n';
        printUsage(std::cerr);
        return 1;
    }

    int i = 0;
    std::string_view const nnModelFn = argv[++i];
    auto network = xai::nn::NNet::fromFile(nnModelFn);
    if (not network) { return 2; }

    std::string_view const datasetFn = argv[++i];

    std::string_view const verifierName = argv[++i];

    std::string_view const strategiesSpec = argv[++i];

    xspace::Framework::Config config;

    while (true) {
        int const c = getopt(argc, argv, "hvr");
        if (c == -1) { break; }

        switch (c) {
            case 'h':
                printUsage();
                return 0;
            case 'v':
                config.beVerbose();
                break;
            case 'r':
                config.reverseVarOrdering();
                break;
            default: /* '?' */
                printUsage(std::cerr);
                return 1;
        }
    }

    auto const dataset = xspace::Dataset{datasetFn};
    std::size_t const size = dataset.size();

    xspace::Framework framework{config};
    framework.setNetwork(std::move(network));
    framework.setVerifier(verifierName);
    std::istringstream strategiesSpecIss{std::string{strategiesSpec}};
    framework.setExpandStrategies(strategiesSpecIss);

    auto explanations = framework.explain(dataset);
    assert(explanations.size() == size);

    return 0;
} catch (std::exception const &) {
    // !!!
    //- ...
}
