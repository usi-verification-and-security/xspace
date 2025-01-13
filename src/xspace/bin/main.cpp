#include <xspace/framework/Config.h>
#include <xspace/framework/Framework.h>
#include <xspace/framework/expand/strategy/Strategies.h>
#include <xspace/framework/explanation/Explanation.h>
#include <xspace/nn/Dataset.h>

#include <iomanip>
#include <iostream>
#include <optional>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include <getopt.h>

namespace {
void printUsageStrategyRow(std::ostream & os, std::string_view name, std::vector<std::string_view> params = {}) {
    os << std::setw(12) << name << ":";
    if (not params.empty()) {
        os << " " << params.front();
        for (auto param : params | std::views::drop(1)) {
            os << ", " << param;
        }
    }
    os << '\n';
}

void printUsageOptRow(std::ostream & os, char opt, std::string_view arg, std::string_view desc) {
    os << "    -" << opt << ' ';
    os << std::left << std::setw(7);
    if (arg.empty()) {
        os << ' ';
    } else {
        os << arg;
    }
    os << desc << '\n';
}

void printUsage(std::ostream & os = std::cout) {
    using xspace::Framework;
    using xspace::expand::opensmt::InterpolationStrategy;

    os << "USAGE: xspace <nn_model_fn> <dataset_fn> <verifier_name> <exp_strategies_spec> [<options>]\n";

    os << "VERIFIERS: opensmt";
#ifdef MARABOU
    os << " marabou";
#endif
    os << '\n';

    os << "STRATEGIES SPEC: '<spec1>[; <spec2>]...'\n";
    os << "Each spec: '<name>[ <param>[, <param>]...]'\n";
    os << "Strategies and parameters:\n";
    //+ template by the strategy and move the params to the classes as well
    printUsageStrategyRow(os, Framework::Expand::AbductiveStrategy::name());
    printUsageStrategyRow(os, Framework::Expand::UnsatCoreStrategy::name(), {"sample", "interval"});
    printUsageStrategyRow(os, Framework::Expand::TrialAndErrorStrategy::name(), {"n <int>"});
    printUsageStrategyRow(os, InterpolationStrategy::name(),
                          {"weak", "strong", "weaker", "stronger", "bweak", "bstrong", "aweak", "astrong", "aweaker",
                           "astronger", "vars x<i>..."});

    os << "OPTIONS:\n";
    printUsageOptRow(os, 'h', "", "Prints this help message and exits");
    printUsageOptRow(os, 'v', "", "Run in verbose mode");
    printUsageOptRow(os, 'r', "", "Reverse the order of variables");
    printUsageOptRow(os, 's', "", "Print the resulting explanations in the SMT-LIB2 format");
    printUsageOptRow(os, 'i', "", "Print the resulting explanations in the form of intervals");
    printUsageOptRow(os, 'n', "<int>", "Maximum no. samples to be processed");
    printUsageOptRow(os, 'S', "", "Shuffle samples");

    os << "\nEXAMPLES:\n";
    os << "xspace models/Heart_attack/heartAttack50.nnet data/heartAttack.csv opensmt ucore\n";
    os << "xspace models/Heart_attack/heartAttack50.nnet data/heartAttack.csv opensmt 'ucore interval' -rvs\n";
    os << "xspace models/Heart_attack/heartAttack50.nnet data/heartAttack.csv opensmt 'ucore; itp aweaker, bstrong'\n";
    os << "xspace models/Heart_attack/heartAttack50.nnet data/heartAttack.csv opensmt 'trial n 2' -n1\n";

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

    int selectedLongOpt = 0;
    // constexpr int versionLongOpt = 1;
    constexpr int formatLongOpt = 2;
    constexpr int filterLongOpt = 3;

    //+ not documented
    struct ::option longOptions[] = {{"help", no_argument, nullptr, 'h'},
                                     {"verbose", no_argument, nullptr, 'v'},
                                     // {"version", no_argument, &selectedLongOpt, versionLongOpt},
                                     {"reverse-var", no_argument, nullptr, 'r'},
                                     {"format", required_argument, &selectedLongOpt, formatLongOpt},
                                     {"shuffle-samples", no_argument, nullptr, 'S'},
                                     {"max-samples", required_argument, nullptr, 'n'},
                                     {"filter-samples", required_argument, &selectedLongOpt, filterLongOpt},
                                     {0, 0, 0, 0}};

    while (true) {
        int optIndex = 0;
        int c = getopt_long(argc, argv, ":hvrsiSn:", longOptions, &optIndex);
        if (c == -1) { break; }

        switch (c) {
            case 0: {
                std::string_view optargStr{optarg};
                switch (selectedLongOpt) {
                    case formatLongOpt:
                        if (optargStr == "smtlib2") {
                            config.printIntervalExplanationsInSmtLib2Format();
                        } else if (optargStr == "intervals") {
                            config.printIntervalExplanationsInIntervalFormat();
                        } else {
                            assert(optargStr == "bounds");
                            config.printingIntervalExplanationsInBoundFormat();
                        }
                        break;
                    case filterLongOpt:
                        std::optional<bool> optCorrectnessFilter{};
                        if (optargStr.starts_with("in")) {
                            optargStr.remove_prefix(2);
                            optCorrectnessFilter = false;
                        }
                        if (optargStr == "correct") {
                            if (not optCorrectnessFilter.has_value()) { optCorrectnessFilter = true; }
                        } else {
                            assert(not optCorrectnessFilter.has_value());
                        }
                        if (optCorrectnessFilter.has_value()) {
                            if (*optCorrectnessFilter) {
                                config.filterCorrectSamples();
                            } else {
                                config.filterIncorrectSamples();
                            }
                            break;
                        }

                        if (optargStr.starts_with("class")) {
                            optargStr.remove_prefix(5);
                            xspace::Dataset::Classification c;
                            c.label = std::stoi(std::string{optargStr});
                            config.filterSamplesOfExpectedClass(std::move(c));
                            break;
                        }

                        assert(false);
                }
                break;
            }
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
            case 'S':
                config.shuffleSamples();
                break;
            case 'n': {
                auto const n = std::stoull(optarg);
                config.setMaxSamples(n);
                break;
            }
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
