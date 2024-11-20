#ifndef XSPACE_CONFIG_H
#define XSPACE_CONFIG_H

#include "Framework.h"

#include <xspace/explanation/Explanation.h>

namespace xspace {
class Framework::Config {
public:
    using Verbosity = short;

    using ExplanationPrintFormat = IntervalExplanation::PrintFormat;

    void setVerbosity(Verbosity verb) { verbosity = verb; }
    void beVerbose() { setVerbosity(1); }

    void reverseVarOrdering() { reverseVarOrder = true; }

    void setPrintExplanationsFormat(ExplanationPrintFormat tp) { explanationPrintFormat = tp; }
    void printExplanationsInBoundFormat() { setPrintExplanationsFormat(ExplanationPrintFormat::bounds); }
    void printExplanationsInSmtLib2Format() { setPrintExplanationsFormat(ExplanationPrintFormat::smtlib2); }
    void printExplanationsInIntervalFormat() { setPrintExplanationsFormat(ExplanationPrintFormat::intervals); }

    Verbosity getVerbosity() const { return verbosity; }
    bool isVerbose() const { return getVerbosity() > 0; }

    bool isReverseVarOrdering() const { return reverseVarOrder; }

    ExplanationPrintFormat const & getPrintingExplanationsFormat() const { return explanationPrintFormat; }
    bool printingExplanationsInBoundFormat() const { return explanationPrintFormat == ExplanationPrintFormat::bounds; }
    bool printingExplanationsInSmtLib2Format() const {
        return explanationPrintFormat == ExplanationPrintFormat::smtlib2;
    }
    bool printingExplanationsInIntervalFormat() const {
        return explanationPrintFormat == ExplanationPrintFormat::intervals;
    }

protected:
    Verbosity verbosity{};

    bool reverseVarOrder{};

    ExplanationPrintFormat explanationPrintFormat{};
};
} // namespace xspace

#endif // XSPACE_CONFIG_H
