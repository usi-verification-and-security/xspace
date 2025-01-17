#include "UnsatCoreStrategy.h"

#include <verifiers/opensmt/OpenSMTVerifier.h>

#include <api/MainSolver.h>

namespace xspace::expand::opensmt {
void UnsatCoreStrategy::executeInit(std::unique_ptr<Explanation> & explanationPtr) {
    Base::executeInit(explanationPtr);
    // if opensmt Strategy also has executeInit, only its local function should be called

    auto & verifier = getVerifier();
    auto & solver = verifier.getSolver();
    auto & solverConf = solver.getConfig();

    using ::opensmt::SMTConfig;
    using ::opensmt::SMTOption;

    char const * msg = "ok";
    solverConf.setOption(SMTConfig::o_produce_unsat_cores, SMTOption(true), msg);
    solverConf.setOption(SMTConfig::o_minimal_unsat_cores, SMTOption(config.minimal), msg);
}
} // namespace xspace::expand::opensmt
