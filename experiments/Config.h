#ifndef XAI_SMT_EXPERIMENTS_CONFIG_H
#define XAI_SMT_EXPERIMENTS_CONFIG_H

namespace xai::experiments {
struct Config {
    enum class ExplanationType { simple = 0, general, opensmt };

    //- static constexpr ExplanationType explanationType = ExplanationType::simple;
    //- static constexpr ExplanationType explanationType = ExplanationType::general;
    static constexpr ExplanationType explanationType = ExplanationType::opensmt;

    static constexpr bool samplesOnly = false;
    //- static constexpr bool samplesOnly = true;

    static constexpr bool minimalUnsatCore = true;
    //- static constexpr bool minimalUnsatCore = false;

    static constexpr bool reverseFeatures = false;
    //- static constexpr bool reverseFeatures = true;
};
}

#endif //XAI_SMT_EXPERIMENTS_CONFIG_H
