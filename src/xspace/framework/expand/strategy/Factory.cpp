#include "Factory.h"

#include "Strategies.h"

#include <xspace/common/Macro.h>
#include <xspace/common/String.h>

#include <queue>
#include <sstream>
#include <stdexcept>

namespace xspace {
Framework::Expand::Strategy::Factory::Factory(Expand & exp, VarOrdering const & order)
    : expand{exp},
      varOrdering{std::move(order)} {}

std::unique_ptr<Framework::Expand::Strategy> Framework::Expand::Strategy::Factory::parse(std::string const & str) {
    std::istringstream iss{str};
    std::string name;
    iss >> name;
    std::queue<std::string> params;
    for (std::string param; iss >> param;) {
        params.push(std::move(param));
    }

    auto const nameLower = toLower(name);

    if (nameLower == AbductiveStrategy::name()) { return parseAbductive(str, params); }
    if (nameLower == UnsatCoreStrategy::name()) { return parseUnsatCore(str, params); }
    if (nameLower == TrialAndErrorStrategy::name()) { return parseTrial(str, params); }
    if (nameLower == expand::opensmt::InterpolationStrategy::name()) { return parseInterpolation(str, params); }

    throw std::invalid_argument{"Unrecognized strategy name: "s + name};
}

template<typename T>
std::unique_ptr<Framework::Expand::Strategy>
Framework::Expand::Strategy::Factory::parseReturnTp(std::string const & str, auto const & params,
                                                    auto &&... args) const {
    checkAdditionalParametersTp<T>(str, params);
    return newStrategyTp<T>(FORWARD(args)...);
}

template<typename T>
std::unique_ptr<Framework::Expand::Strategy>
Framework::Expand::Strategy::Factory::newStrategyTp(auto &&... args) const {
    return std::make_unique<T>(expand, FORWARD(args)..., varOrdering);
}

template<typename T>
void Framework::Expand::Strategy::Factory::checkAdditionalParametersTp(std::string const & str,
                                                                       auto const & params) const {
    if (params.empty()) { return; }
    throwAdditionalParametersTp<T>(str);
};

template<typename T>
void Framework::Expand::Strategy::Factory::throwAdditionalParametersTp(std::string const & str) const {
    throw std::invalid_argument{throwMessageTp<T>("Additional parameters: "s + str)};
}

template<typename T>
void Framework::Expand::Strategy::Factory::throwInvalidParameterTp(std::string const & param) const {
    throw std::invalid_argument{throwMessageTp<T>("Invalid parameter: "s + param)};
}

template<typename T>
std::string Framework::Expand::Strategy::Factory::throwMessageTp(std::string const & msg) const {
    return "Strategy "s + T::name() + ": " + msg;
}

std::unique_ptr<Framework::Expand::Strategy>
Framework::Expand::Strategy::Factory::parseAbductive(std::string const & str, auto & params) {
    return parseReturnTp<AbductiveStrategy>(str, params);
}

std::unique_ptr<Framework::Expand::Strategy>
Framework::Expand::Strategy::Factory::parseUnsatCore(std::string const & str, auto & params) {
    using expand::opensmt::UnsatCoreStrategy;

    UnsatCoreStrategy::Config conf;
    while (not params.empty()) {
        auto param = std::move(params.front());
        params.pop();
        auto const paramLower = toLower(param);
        if (paramLower == "sample") {
            conf.splitEq = false;
            break;
        }

        if (paramLower == "interval") {
            conf.splitEq = true;
            break;
        }

        throwInvalidParameterTp<UnsatCoreStrategy>(param);
    }

    return parseReturnTp<UnsatCoreStrategy>(str, params, conf);
}

std::unique_ptr<Framework::Expand::Strategy> Framework::Expand::Strategy::Factory::parseTrial(std::string const & str,
                                                                                              auto & params) {
    TrialAndErrorStrategy::Config conf;
    bool maxAttemptsFollows = false;
    while (not params.empty()) {
        auto param = std::move(params.front());
        params.pop();
        if (maxAttemptsFollows) {
            conf.maxAttempts = std::stoi(param);
            break;
        }
        auto const paramLower = toLower(param);
        if (paramLower == "n") {
            maxAttemptsFollows = true;
            continue;
        }

        throwInvalidParameterTp<TrialAndErrorStrategy>(param);
    }

    return parseReturnTp<TrialAndErrorStrategy>(str, params, conf);
}

std::unique_ptr<Framework::Expand::Strategy>
Framework::Expand::Strategy::Factory::parseInterpolation(std::string const & str, auto & params) {
    using expand::opensmt::InterpolationStrategy;

    InterpolationStrategy::Config conf;
    while (not params.empty()) {
        auto param = std::move(params.front());
        params.pop();
        auto const paramLower = toLower(param);
        if (paramLower == "weak") {
            conf.boolInterpolationAlg = InterpolationStrategy::BoolInterpolationAlg::weak;
            conf.arithInterpolationAlg = InterpolationStrategy::ArithInterpolationAlg::weak;
            break;
        }

        if (paramLower == "strong") {
            conf.boolInterpolationAlg = InterpolationStrategy::BoolInterpolationAlg::strong;
            conf.arithInterpolationAlg = InterpolationStrategy::ArithInterpolationAlg::strong;
            break;
        }

        if (paramLower == "weaker") {
            conf.boolInterpolationAlg = InterpolationStrategy::BoolInterpolationAlg::weak;
            conf.arithInterpolationAlg = InterpolationStrategy::ArithInterpolationAlg::weaker;
            break;
        }

        if (paramLower == "stronger") {
            conf.boolInterpolationAlg = InterpolationStrategy::BoolInterpolationAlg::strong;
            conf.arithInterpolationAlg = InterpolationStrategy::ArithInterpolationAlg::stronger;
            break;
        }

        if (paramLower == "bweak") {
            conf.boolInterpolationAlg = InterpolationStrategy::BoolInterpolationAlg::weak;
            continue;
        }

        if (paramLower == "bstrong") {
            conf.boolInterpolationAlg = InterpolationStrategy::BoolInterpolationAlg::strong;
            continue;
        }

        if (paramLower == "aweak") {
            conf.arithInterpolationAlg = InterpolationStrategy::ArithInterpolationAlg::weak;
            continue;
        }

        if (paramLower == "astrong") {
            conf.arithInterpolationAlg = InterpolationStrategy::ArithInterpolationAlg::strong;
            continue;
        }

        if (paramLower == "aweaker") {
            conf.arithInterpolationAlg = InterpolationStrategy::ArithInterpolationAlg::weaker;
            continue;
        }

        if (paramLower == "astronger") {
            conf.arithInterpolationAlg = InterpolationStrategy::ArithInterpolationAlg::stronger;
            continue;
        }

        throwInvalidParameterTp<InterpolationStrategy>(param);
    }

    return parseReturnTp<InterpolationStrategy>(str, params, conf);
}
} // namespace xspace
