#ifndef XSPACE_EXPAND_H
#define XSPACE_EXPAND_H

#include "../Framework.h"

#include <xspace/common/Var.h>
#include <xspace/nn/Dataset.h>

#include <memory>
#include <vector>

namespace xai::verifiers {
class Verifier;
}

namespace xspace {
class Explanation;

class Framework::Expand {
public:
    struct VarOrdering {
        enum class Type { regular, reverse, manual };

        using ManualOrder = std::vector<VarIdx>;

        Type type{Type::regular};
        ManualOrder manualOrder{};
    };

    class Strategy;

    using Strategies = std::vector<std::unique_ptr<Strategy>>;

    Expand(Framework &);

    Framework const & getFramework() const { return framework; }

    void setVerifier(std::string_view name);

    xai::verifiers::Verifier const & getVerifier() const {
        assert(verifierPtr);
        return *verifierPtr;
    }

    void setStrategies(std::istream &);

    void operator()(Explanations &, Dataset &);

protected:
    class AbductiveStrategy;
    class TrialAndErrorStrategy;
    class UnsatCoreStrategy;

    static std::unique_ptr<xai::verifiers::Verifier> makeVerifier(std::string_view name);
    void setVerifier(std::unique_ptr<xai::verifiers::Verifier>);

    void addStrategy(std::unique_ptr<Strategy>);

    void initVerifier();

    void assertModel();
    void resetModel();

    void assertClassification(Dataset::Output const &);
    void resetClassification();

    void printStatsHead(Dataset const &) const;
    void printStats(Explanation const &, Dataset const &, Dataset::Sample::Idx) const;

    Framework & framework;

    std::unique_ptr<xai::verifiers::Verifier> verifierPtr{};

    Strategies strategies{};

    bool isAbductiveOnly{true};
};
} // namespace xspace

#endif // XSPACE_EXPAND_H
