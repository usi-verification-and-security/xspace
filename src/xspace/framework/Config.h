#ifndef XSPACE_CONFIG_H
#define XSPACE_CONFIG_H

#include "Framework.h"
#include "explanation/IntervalExplanation.h"

namespace xspace {
//+ move parsing cmdline options here
//+ after construction, make the conf const
class Framework::Config {
public:
    using Verbosity = short;

    void setVerbosity(Verbosity verb) { verbosity = verb; }
    void beVerbose() { setVerbosity(1); }

    void reverseVarOrdering() { reverseVarOrder = true; }

    void setPrintIntervalExplanationsFormat(IntervalExplanation::PrintFormat tp) {
        intervalExplanationPrintFormat = tp;
    }
    void printIntervalExplanationsInSmtLib2Format() {
        setPrintIntervalExplanationsFormat(IntervalExplanation::PrintFormat::smtlib2);
    }
    void printIntervalExplanationsInBoundFormat() {
        setPrintIntervalExplanationsFormat(IntervalExplanation::PrintFormat::bounds);
    }
    void printIntervalExplanationsInIntervalFormat() {
        setPrintIntervalExplanationsFormat(IntervalExplanation::PrintFormat::intervals);
    }

    void setMaxSamples(std::size_t n) { maxSamples = n; }

    Verbosity getVerbosity() const { return verbosity; }
    bool isVerbose() const { return getVerbosity() > 0; }

    bool isReverseVarOrdering() const { return reverseVarOrder; }

    IntervalExplanation::PrintFormat const & getPrintingIntervalExplanationsFormat() const {
        return intervalExplanationPrintFormat;
    }
    bool printingIntervalExplanationsInSmtLib2Format() const {
        return intervalExplanationPrintFormat == IntervalExplanation::PrintFormat::smtlib2;
    }
    bool printingIntervalExplanationsInBoundFormat() const {
        return intervalExplanationPrintFormat == IntervalExplanation::PrintFormat::bounds;
    }
    bool printingIntervalExplanationsInIntervalFormat() const {
        return intervalExplanationPrintFormat == IntervalExplanation::PrintFormat::intervals;
    }

    std::size_t getMaxSamples() const { return maxSamples; }
    bool limitingMaxSamples() const { return getMaxSamples() > 0; }

protected:
    Verbosity verbosity{};

    bool reverseVarOrder{};

    IntervalExplanation::PrintFormat intervalExplanationPrintFormat{IntervalExplanation::PrintFormat::bounds};

    std::size_t maxSamples{};
};
} // namespace xspace

#endif // XSPACE_CONFIG_H
