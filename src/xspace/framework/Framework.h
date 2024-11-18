#ifndef XSPACE_FRAMEWORK_H
#define XSPACE_FRAMEWORK_H

#include <xspace/common/Var.h>

#include <nn/NNet.h>

#include <cassert>
#include <memory>
#include <string_view>
#include <vector>

namespace xai::nn {
class NNet;
}

namespace xspace {
class Dataset;
class IntervalExplanation;

// Class that represents the Space Explanation Framework
class Framework {
public:
    class Config;

    // Not inline because of fwd-decl. types
    Framework();
    Framework(Config const &);
    ~Framework();

    Config const & getConfig() const { return *configPtr; }

    void setNetwork(std::unique_ptr<xai::nn::NNet> nn) {
        assert(nn);
        networkPtr = std::move(nn);
    }

    void setVerifier(std::string_view name);

    void setExpandStrategies(std::string_view spec);

    xai::nn::NNet const & getNetwork() const {
        assert(networkPtr);
        return *networkPtr;
    }

    std::vector<IntervalExplanation> explain(Dataset const &);

    // Valid only after calling `explain`
    std::size_t varSize() const { return varNames.size(); }
    VarName const & varName(VarIdx idx) const { return varNames[idx]; }

protected:
    class Expand;

    using VarNames = std::vector<VarName>;

    void setVarNames(Dataset const &);

    std::vector<IntervalExplanation> encodeSamples(Dataset const &);

    std::unique_ptr<Config> configPtr;

    std::unique_ptr<xai::nn::NNet> networkPtr{};

    std::unique_ptr<Expand> expandPtr{};

    VarNames varNames{};
};
} // namespace xspace

#endif // XSPACE_FRAMEWORK_H
