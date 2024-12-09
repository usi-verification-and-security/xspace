#ifndef XSPACE_FRAMEWORK_H
#define XSPACE_FRAMEWORK_H

#include <xspace/common/Bound.h>
#include <xspace/common/Var.h>

#include <nn/NNet.h>

#include <cassert>
#include <iosfwd>
#include <memory>
#include <string_view>
#include <vector>

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

    Config const & getConfig() const {
        assert(configPtr);
        return *configPtr;
    }

    void setNetwork(std::unique_ptr<xai::nn::NNet>);

    void setVerifier(std::string_view name);

    void setExpandStrategies(std::istream &);

    xai::nn::NNet const & getNetwork() const {
        assert(networkPtr);
        return *networkPtr;
    }

    std::size_t varSize() const { return varNames.size(); }
    VarName const & getVarName(VarIdx idx) const { return varNames[idx]; }

    Interval const & getDomainInterval(VarIdx idx) const { return domainIntervals[idx]; }

    std::vector<IntervalExplanation> explain(Dataset const &);

protected:
    class Expand;

    class Print;

    using VarNames = std::vector<VarName>;

    std::vector<IntervalExplanation> encodeSamples(Dataset const &);

    std::unique_ptr<Config> configPtr;

    std::unique_ptr<xai::nn::NNet> networkPtr{};
    VarNames varNames{};
    std::vector<Interval> domainIntervals{};

    std::unique_ptr<Expand> expandPtr{};

    std::unique_ptr<Print> printPtr;
};
} // namespace xspace

#endif // XSPACE_FRAMEWORK_H
