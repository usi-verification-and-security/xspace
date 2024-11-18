#include <xspace/explanation/Explanation.h>
#include <xspace/framework/Config.h>
#include <xspace/framework/Framework.h>
#include <xspace/nn/Dataset.h>

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

void printUsage(std::ostream & os = std::cout) {
    os << "USAGE: xspace <nn_model_fn> <dataset_fn> <verifier_name> <exp_strategies_spec> [<options>]" << std::endl;
}

int main(int argc, char * argv[]) try {
    constexpr int minArgs = 4;
    constexpr int maxArgs = 5;
    static_assert(minArgs <= maxArgs);

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
    if (nArgs > maxArgs) {
        std::cerr << "Expected at most " << maxArgs << " arguments, got: " << nArgs << '\n';
        printUsage(std::cerr);
        return 1;
    }

    int i = 0;
    std::string_view nnModelFn = argv[++i];
    auto network = xai::nn::NNet::fromFile(nnModelFn);
    if (not network) { return 2; }

    xspace::Framework::Config config;

    // !!!
    config.setVerbose();

    std::string_view datasetFn = argv[++i];
    auto const dataset = xspace::Dataset{datasetFn};
    std::size_t const size = dataset.size();

    xspace::Framework framework{config};
    framework.setNetwork(std::move(network));

    std::string_view verifierName = argv[++i];
    framework.setVerifier(verifierName);

    std::string_view strategiesSpec = argv[++i];
    std::istringstream strategiesSpecIss{std::string{strategiesSpec}};
    framework.setExpandStrategies(strategiesSpecIss);

    auto explanations = framework.explain(dataset);
    assert(explanations.size() == size);

    return 0;
} catch (std::exception const &) {
    // !!!
    //- ...
}
