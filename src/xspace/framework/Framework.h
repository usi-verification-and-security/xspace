#ifndef XSPACE_FRAMEWORK_H
#define XSPACE_FRAMEWORK_H

#include <xspace/common/Interval.h>
#include <xspace/common/Var.h>

#include <nn/NNet.h>

#include <cassert>
#include <iosfwd>
#include <memory>
#include <string_view>
#include <vector>

namespace xspace {
class Dataset;
class Explanation;

using Explanations = std::vector<std::unique_ptr<Explanation>>;

// Class that represents the Space Explanation Framework
class Framework {
public:
    class Config;

    // Not inline because of fwd-decl. types
    Framework();
    Framework(Config const &);
    Framework(Config const &, std::unique_ptr<xai::nn::NNet>);
    Framework(Config const &, std::unique_ptr<xai::nn::NNet>, std::string_view verifierName,
              std::istream & expandStrategiesSpec);
    ~Framework();

    void setConfig(Config const &);

    Config const & getConfig() const {
        assert(configPtr);
        return *configPtr;
    }

    void setNetwork(std::unique_ptr<xai::nn::NNet>);

    xai::nn::NNet const & getNetwork() const {
        assert(networkPtr);
        return *networkPtr;
    }

    void setExpand(std::string_view verifierName, std::istream & strategiesSpec);

    std::size_t varSize() const { return varNames.size(); }
    VarName const & getVarName(VarIdx idx) const { return varNames[idx]; }

    Interval const & getDomainInterval(VarIdx idx) const { return domainIntervals[idx]; }

    Explanations explain(Dataset const &);

protected:
    class Expand;

    class Print;

    using VarNames = std::vector<VarName>;

    Explanations encodeSamples(Dataset const &);

    std::unique_ptr<Config> configPtr;

    std::unique_ptr<xai::nn::NNet> networkPtr{};
    VarNames varNames{};
    std::vector<Interval> domainIntervals{};

    std::unique_ptr<Expand> expandPtr{};

    std::unique_ptr<Print> printPtr{};
};
} // namespace xspace

#endif // XSPACE_FRAMEWORK_H
