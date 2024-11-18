#ifndef XSPACE_EXPAND_H
#define XSPACE_EXPAND_H

#include "../Framework.h"

#include <xspace/common/Core.h>
#include <xspace/common/Var.h>

#include <memory>
#include <vector>

namespace xai::verifiers {
class Verifier;
}

namespace xspace {
class Bound;
class Dataset;
class IntervalExplanation;

class Framework::Expand {
public:
    struct VarOrdering {
        enum class Type { regular = 0, reverse, manual };

        using ManualOrder = std::vector<VarIdx>;

        Type type{};
        ManualOrder manualOrder{};
    };

    struct Output {
        using Values = std::vector<Float>;

        Values values;
        VarIdx classifiedIdx;
    };

    using Outputs = std::vector<Output>;

    Expand(Framework & fw) : framework{fw} {}

    void setVerifier(std::string_view name) { setVerifier(makeVerifier(name)); }

    void setStrategies(std::istream &);

    void setOutputs(Outputs outs) { outputs = std::move(outs); }

    xai::verifiers::Verifier const & getVerifier() const {
        assert(verifierPtr);
        return *verifierPtr;
    }

    void operator()(std::vector<IntervalExplanation> &, Dataset const &);

protected:
    class Strategy;
    class AbductiveStrategy;
    class UnsatCoreStrategy;

    static std::unique_ptr<xai::verifiers::Verifier> makeVerifier(std::string_view name);
    void setVerifier(std::unique_ptr<xai::verifiers::Verifier> vf) {
        assert(vf);
        verifierPtr = std::move(vf);
    }

    void addStrategy(std::unique_ptr<Strategy> strategy) { strategies.push_back(std::move(strategy)); }

    void assertModel();

    void assertClassification(Output const &);

    void assertBound(VarIdx, Bound const &);

    bool checkFormsExplanation();

    Framework & framework;

    std::unique_ptr<xai::verifiers::Verifier> verifierPtr{};

    std::vector<std::unique_ptr<Strategy>> strategies{};

    Outputs outputs{};
};
} // namespace xspace

#endif // XSPACE_EXPAND_H
