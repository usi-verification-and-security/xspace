#ifndef XSPACE_EXPAND_H
#define XSPACE_EXPAND_H

#include "../Framework.h"

#include <xspace/common/Bound.h>
#include <xspace/common/Core.h>
#include <xspace/common/Var.h>

#include <memory>
#include <vector>

namespace xai::verifiers {
class Verifier;
}

namespace xspace {
class VarBound;
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

    void addStrategy(std::unique_ptr<Strategy>);

    void initVerifier();

    void assertModel();
    void resetModel();

    void assertClassification(Output const &);
    void resetClassification();

    void assertVarBound(VarIdx, VarBound const &, bool splitEq = false);
    void assertInterval(VarIdx, Interval const &);
    void assertInnerInterval(VarIdx, Interval const &);
    void assertInnerInterval(VarIdx, LowerBound const &, UpperBound const &);
    void assertPoint(VarIdx, Float);
    void assertPoint(VarIdx, EqBound const &, bool splitEq);
    void assertPointNoSplit(VarIdx, EqBound const &);
    void assertPointSplit(VarIdx, EqBound const &);

    void assertBound(VarIdx, Bound const &);
    void assertEquality(VarIdx, EqBound const &);
    void assertLowerBound(VarIdx, LowerBound const &);
    void assertUpperBound(VarIdx, UpperBound const &);

    bool checkFormsExplanation();

    void printStatsHead(Dataset const &) const;
    void printStats(IntervalExplanation const &, Dataset const &, std::size_t i) const;

    Framework & framework;

    std::unique_ptr<xai::verifiers::Verifier> verifierPtr{};

    std::vector<std::unique_ptr<Strategy>> strategies{};

    Outputs outputs{};

    bool isAbductiveOnly{true};
};
} // namespace xspace

#endif // XSPACE_EXPAND_H
