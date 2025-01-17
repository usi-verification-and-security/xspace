#ifndef XSPACE_PARSE_H
#define XSPACE_PARSE_H

#include "Framework.h"

namespace xspace {
class Dataset;

class Framework::Parse {
public:
    Parse(Framework &);

    Explanations parseIntervalExplanations(std::string_view fileName, Dataset const &) const;

protected:
    Explanations parseIntervalExplanationsSmtLib2(std::istream &, Dataset const &) const;
    std::unique_ptr<Explanation> parseIntervalExplanationSmtLib2(std::istream &) const;

    Framework & framework;
};
} // namespace xspace

#endif // XSPACE_PARSE_H
