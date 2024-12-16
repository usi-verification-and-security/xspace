#include <xspace/framework/Config.h>
#include <xspace/framework/Framework.h>
#include <xspace/framework/explanation/Explanation.h>
#include <xspace/nn/Dataset.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <getopt.h>

namespace {
void printUsageStrategyRow(std::ostream & os, std::string_view name, std::vector<std::string_view> params = {}) {
    os << "    " << name;
    if (not params.empty()) {
        os << "\tPARAMS:";
        for (auto param : params) {
            os << "  " << param;
        }
    }
    os << '\n';
}

void printUsageOptRow(std::ostream & os, char opt, std::string_view desc) {
    os << "    -" << opt << "      " << desc << '\n';
}

void printUsage(std::ostream & os = std::cout) {
    os << "USAGE: xspace <nn_model_fn> <dataset_fn> <verifier_name> <exp_strategies_spec> [<options>]\n";

    os << "VERIFIERS: opensmt";
#ifdef MARABOU
    os << " marabou";
#endif
    os << '\n';

    os << "STRATEGIES SPEC: '<spec1>[,<spec2>]...'\n";
    os << "Each spec: '<name>[ <param>]...'\n";
    printUsageStrategyRow(os, "abductive");
    printUsageStrategyRow(os, "ucore", {"sample", "interval"});
    printUsageStrategyRow(os, "trial", {"n <int>"});
    printUsageStrategyRow(os, "itp");

    os << "OPTIONS:\n";
    printUsageOptRow(os, 'h', "Prints this help message and exits");
    printUsageOptRow(os, 'v', "Run in verbose mode");
    printUsageOptRow(os, 'r', "Reverse the order of variables");
    printUsageOptRow(os, 's', "Print the resulting explanations in the SMT-LIB2 format");
    printUsageOptRow(os, 'i', "Print the resulting explanations in the form of intervals");
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
        int const c = getopt(argc, argv, ":hvrsi");
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
            case 's':
                config.printIntervalExplanationsInSmtLib2Format();
                break;
            case 'i':
                config.printIntervalExplanationsInIntervalFormat();
                break;
            default:
                assert(c == '?');
                std::cerr << "Unrecognized option: '-" << char(optopt) << "'\n\n";
                printUsage(std::cerr);
                return 1;
        }
    }

    auto dataset = xspace::Dataset{datasetFn};
    std::size_t const size = dataset.size();

    std::istringstream strategiesSpecIss{std::string{strategiesSpec}};
    xspace::Framework framework{config, std::move(network), verifierName, strategiesSpecIss};

    auto explanations = framework.explain(dataset);
    assert(explanations.size() == size);

    return 0;
} catch (std::system_error const & e) {
    std::cerr << e.what() << '\n' << std::endl;
    printUsage(std::cerr);
    return e.code().value();
} catch (std::exception const & e) {
    std::cerr << e.what() << '\n' << std::endl;
    printUsage(std::cerr);
    return 1;
}
