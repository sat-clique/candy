/***************************************************************************************[Main.cc]
 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 LRI  - Univ. Paris Sud, France (2009-2013)
 Labri - Univ. Bordeaux, France

 Syrup (Glucose Parallel) -- Copyright (c) 2013-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 Labri - Univ. Bordeaux, France

 Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
 Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it
 is based on. (see below).

 Glucose-Syrup sources are based on another copyright. Permissions and copyrights for the parallel
 version of Glucose-Syrup (the "Software") are granted, free of charge, to deal with the Software
 without restriction, including the rights to use, copy, modify, merge, publish, distribute,
 sublicence, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 - The above and below copyrights notices and this permission notice shall be included in all
 copies or substantial portions of the Software;
 - The parallel version of Glucose (all files modified since Glucose 3.0 releases, 2013) cannot
 be used in any competitive event (sat competitions/evaluations) without the express permission of
 the authors (Gilles Audemard / Laurent Simon). This is also the case for any competitive event
 using Glucose Parallel as an embedded SAT engine (single core or not).


 --------------- Original Minisat Copyrights

 Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 Copyright (c) 2007-2010, Niklas Sorensson

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#include <errno.h>
#include <iostream>
#include <signal.h>
#include <zlib.h>
#include <string>
#include <stdexcept>
#include <regex>
#include <functional>
#include <type_traits>
#include <chrono>

#include <candy/utils/CNFProblem.h>
#include "candy/core/Certificate.h"
#include "candy/core/Statistics.h"
#include "candy/core/ClauseAllocator.h"
#include "candy/utils/System.h"
#include "candy/utils/ParseUtils.h"
#include "candy/utils/Options.h"
#include "candy/utils/StringUtils.h"
#include "candy/utils/MemUtils.h"
#include "candy/simp/SimpSolver.h"

#include "candy/gates/GateAnalyzer.h"
#include "candy/gates/MiterDetector.h"
#include "candy/rsar/ARSolver.h"
#include "candy/rsar/SolverAdapter.h"
#include "candy/rsar/Heuristics.h"
#include "candy/randomsimulation/RandomSimulator.h"
#include "candy/randomsimulation/Conjectures.h"
#include "candy/randomsimulation/ClauseOrder.h"
#include "candy/randomsimulation/SimulationVector.h"

#include "candy/rsil/BranchingHeuristics.h"

#if !defined(WIN32)
#include <sys/resource.h>
#endif

using namespace Candy;

static std::function<void()> solverSetInterrupt;
static std::function<bool()> solverIsVerbose;
static std::function<std::pair<uint64_t, uint64_t>()> solverGetNConflictsAndProps;

// Terminate by notifying the solver and back out gracefully. This is mainly to have a test-case
// for this feature of the Solver as it may take longer than an immediate call to '_exit()'.
static void SIGINT_interrupt(int signum) {
    //solver->setInterrupt(true);
    assert(solverSetInterrupt);
    solverSetInterrupt();
}

// Note that '_exit()' rather than 'exit()' has to be used. The reason is that 'exit()' calls
// destructors and may cause deadlocks if a malloc/free function happens to be running (these
// functions are guarded by locks for multithreaded use).
static void SIGINT_exit(int signum) {
    printf("\n*** INTERRUPTED ***\n");
    if (solverIsVerbose && solverIsVerbose()) {
        assert(solverGetNConflictsAndProps);
        auto statistics = solverGetNConflictsAndProps();
        Statistics::getInstance().printFinalStats(statistics.first /* conflicts */,
                                                  statistics.second /* propagations */);
    }
    _exit(1);
}

static void setLimits(int cpu_lim, int mem_lim) {
#if !defined(CANDY_HAVE_RLIMIT)
  #if defined(_MSC_VER)
    #pragma message ("Warning: setting limits not yet implemented for non-POSIX platforms")
  #else
    #warning "setting limits not yet implemented for non-POSIX platforms"
  #endif
#else
    // Set limit on CPU-time:
    if (cpu_lim != INT32_MAX) {
        rlimit rl;
        getrlimit(RLIMIT_CPU, &rl);
        if (rl.rlim_max == RLIM_INFINITY || (rlim_t) cpu_lim < rl.rlim_max) {
            rl.rlim_cur = cpu_lim;
            if (setrlimit(RLIMIT_CPU, &rl) == -1)
                printf("c WARNING! Could not set resource limit: CPU-time.\n");
        }
    }

    // Set limit on virtual memory:
    if (mem_lim != INT32_MAX) {
        rlim_t new_mem_lim = (rlim_t) mem_lim * 1024 * 1024;
        rlimit rl;
        getrlimit(RLIMIT_AS, &rl);
        if (rl.rlim_max == RLIM_INFINITY || new_mem_lim < rl.rlim_max) {
            rl.rlim_cur = new_mem_lim;
            if (setrlimit(RLIMIT_AS, &rl) == -1)
                printf("c WARNING! Could not set resource limit: Virtual memory.\n");
        }
    }
#endif
}

template<class SolverType>
static void printModel(FILE* f, SolverType* solver) {
    fprintf(f, "v");
    for (size_t i = 0; i < solver->nVars(); i++)
        if (solver->model[i] != l_Undef)
            fprintf(f, " %s%zu", (solver->model[i] == l_True) ? "" : "-", i + 1);
    fprintf(f, " 0\n");
}

/**
 * Runs the SAT solver, performing simplification if \p do_preprocess is true.
 */
template<class SolverType>
static lbool solve(SolverType& S, bool do_preprocess) {
    lbool result = l_Undef;

    if (do_preprocess) {
        Statistics::getInstance().runtimeStart(RT_SIMPLIFIER);
        S.eliminate();
        S.disablePreprocessing();
        Statistics::getInstance().runtimeStop(RT_SIMPLIFIER);
        if (S.isInConflictingState()) {
            result = l_False;
            S.certificate->proof();
        }
        if (S.verbosity > 0) {
            Statistics::getInstance().printRuntime(RT_SIMPLIFIER);
            if (result == l_False) {
                printf("c ==============================================================================================\n");
                printf("c Solved by simplification\n");
            }
        }
    }
    if (S.verbosity > 0) {
        printf("c |                                                                                            |\n");
    }

    if (result == l_Undef) {
        vector<Lit> assumptions;
        result = S.solve(assumptions);
    }

    return result;
}

/**
 * Prints the SAT solving result \p result and, if the verbosity level of the solver \p S
 * is greater than 0, print statistics about the solving process.
 *
 * If \p outputFilename is non-null, the result also gets written to the file given by
 * \p outputFilename. The target file gets truncated.
 */
template<class SolverType>
static void printResult(SolverType& S, lbool result, bool showModel, const char* outputFilename = nullptr) {
    if (S.verbosity > 0) {
        Statistics::getInstance().printFinalStats(S.nConflicts, S.nPropagations);
        Statistics::getInstance().printAllocatorStatistics();
        Statistics::getInstance().printRuntime("Runtime Revamp");
        Statistics::getInstance().printRuntime("Runtime Sort Watches");
        Statistics::getInstance().printRuntime("Runtime Simplify");
    }

    printf(result == l_True ? "s SATISFIABLE\n" : result == l_False ? "s UNSATISFIABLE\n" : "s INDETERMINATE\n");

    FILE* res = outputFilename != nullptr ? fopen(outputFilename, "wb") : nullptr;
    if (res != NULL) {
        fprintf(res, result == l_True ? "s SATISFIABLE\n" : result == l_False ? "s UNSATISFIABLE\n" : "s INDETERMINATE\n");
        if (result == l_True) {
            printModel(res, &S);
        }
        fclose(res);
    } else {
        if (showModel && result == l_True) {
            printModel(stdout, &S);
        }
    }
}

/**
 * Installs signal handlers for SIGINT and SIGXCPU.
 * If \p handleInterruptsBySolver is true, the interrupts are handled by SIGINT_interrupt();
 * otherwise, they are set up to be handled by SIGINT_exit().
 */
template<class SolverType>
static void installSignalHandlers(bool handleInterruptsBySolver, SolverType* solver) {
#if defined(WIN32) && !defined(CYGWIN)
#if defined(_MSC_VER)
#pragma message ("Warning: setting signal handlers not yet implemented for Win32")
#else
#warning "setting signal handlers not yet implemented for Win32"
#endif
#else
    if (handleInterruptsBySolver) {
        solverIsVerbose = [solver]() {
            return solver->verbosity > 0;
        };
        
        solverSetInterrupt = [solver]() {
            solver->setInterrupt(true);
        };
        
        solverGetNConflictsAndProps = [solver]() {
            return std::pair<uint64_t, uint64_t>(solver->nConflicts, solver->nPropagations);
        };
        
        signal(SIGINT, SIGINT_interrupt);
        signal(SIGXCPU, SIGINT_interrupt);
    } else {
        signal(SIGINT, SIGINT_exit);
        signal(SIGXCPU, SIGINT_exit);
    }
#endif
}

/**
 * Prints statistics about the problem to be solved.
 */
template<class SolverType>
static void printProblemStatistics(SolverType& S) {
    printf("c ====================================[ Problem Statistics ]====================================\n");
    printf("c |                                                                                            |\n");
    printf("c |  Number of variables:  %12zu                                                        |\n", S.nVars());
    printf("c |  Number of clauses:    %12zu                                                        |\n", S.nClauses());
}

struct GateRecognitionArguments {
    int opt_gr_tries;
    bool opt_gr_patterns;
    bool opt_gr_semantic;
    unsigned int opt_gr_semantic_budget;
    std::chrono::milliseconds opt_gr_timeout;
    bool opt_gr_holistic;
    bool opt_gr_lookahead;
    bool opt_gr_intensify;
    int opt_gr_lookahead_threshold;
    bool opt_print_gates;
};

std::ostream& operator <<(std::ostream& stream, const GateRecognitionArguments& arguments) {
    stream << "c Gate recognition arguments: " << std::endl
    << "c   Max tries: " << arguments.opt_gr_tries << std::endl
    << "c   Patterns: " << arguments.opt_gr_patterns << std::endl
    << "c   Semantic: " << arguments.opt_gr_semantic << std::endl
    << "c   Semantic budget: " << arguments.opt_gr_semantic_budget << std::endl
    << "c   Timeout: " << arguments.opt_gr_timeout.count() << " ms" << std::endl
    << "c   Holistic: " << arguments.opt_gr_holistic << std::endl
    << "c   Lookahead: " << arguments.opt_gr_lookahead << std::endl
    << "c   Intensify: " << arguments.opt_gr_intensify << std::endl
    << "c   Lookahead threshold: " << arguments.opt_gr_lookahead_threshold << std::endl
    << "c   Print gates: " << arguments.opt_print_gates << std::endl;
    return stream;
}

static std::unique_ptr<Candy::GateAnalyzer> createGateAnalyzer(Candy::CNFProblem &dimacs, const GateRecognitionArguments& recognitionArgs) {
    return backported_std::make_unique<Candy::GateAnalyzer>(dimacs, recognitionArgs.opt_gr_tries, recognitionArgs.opt_gr_patterns,
                    recognitionArgs.opt_gr_semantic, recognitionArgs.opt_gr_holistic, recognitionArgs.opt_gr_lookahead, recognitionArgs.opt_gr_intensify,
                    recognitionArgs.opt_gr_lookahead_threshold, recognitionArgs.opt_gr_semantic_budget, recognitionArgs.opt_gr_timeout);
}

/**
 * Performs gate recognition on the problem \p dimacs and prints statistics.
 *
 * TODO: document parameters
 */
static void benchmarkGateRecognition(Candy::CNFProblem &dimacs, const GateRecognitionArguments& recognitionArgs) {
    Statistics::getInstance().runtimeStart(RT_GATOR);
    auto gates = createGateAnalyzer(dimacs, recognitionArgs);
    gates->analyze();
    Statistics::getInstance().runtimeStop(RT_GATOR);
    printf("c =====================================[ Problem Statistics ]===================================\n");
    printf("c |                                                                                            |\n");
    printf("c |  Number of gates:        %12d                                                      |\n", gates->getGateCount());
    printf("c |  Number of variables:    %12d                                                      |\n", dimacs.nVars());
    printf("c |  Number of clauses:      %12d                                                      |\n", dimacs.nClauses());
    Statistics::getInstance().printRuntime(RT_GATOR);
    printf("c |                                                                                            |\n");
    printf("c ==============================================================================================\n");
    if (recognitionArgs.opt_print_gates) {
        gates->printGates();
    }
}

class UnsuitableProblemException {
public:
    
    explicit UnsuitableProblemException(const std::string& what)
    : m_what(what) {
    }
    
    const std::string& what() const noexcept {
        return m_what;
    }
    
private:
    std::string m_what;
};

struct RandomSimulationArguments {
    const int nRounds;
    const bool abortByRRAT;
    const double rrat;
    const bool filterConjecturesBySize;
    const int maxConjectureSize;
    const bool removeBackboneConjectures;
    const bool filterGatesByNonmono;
    std::chrono::milliseconds preprocessingTimeLimit;
};

std::ostream& operator <<(std::ostream& stream, const RandomSimulationArguments& arguments) {
    stream << "c Random simulation arguments: " << std::endl
    << "c   Rounds: " << arguments.nRounds << std::endl
    << "c   RRAT abort heuristic enabled: " << arguments.abortByRRAT << std::endl
    << "c   RRAT: " << arguments.rrat << std::endl
    << "c   Filtering by conjecture size enabled: " << arguments.filterConjecturesBySize << std::endl
    << "c   Max. conjecture size: " << arguments.maxConjectureSize << std::endl
    << "c   Remove backbone conjectures: " << arguments.removeBackboneConjectures << std::endl
    << "c   Remove conjectures about monotonously nested gates: " << arguments.filterGatesByNonmono << std::endl
    << "c   Preprocessing time limit: " << arguments.preprocessingTimeLimit.count() << " ms" << std::endl;
    
    return stream;
}

struct RSARArguments {
    const bool useRSAR;
    const int maxRefinementSteps;
    const Candy::SimplificationHandlingMode simplificationHandlingMode;
    const bool withInputDepCountHeuristic;
    const std::string inputDepCountHeuristicConfiguration;
    const int minGateCount;
};

std::ostream& operator <<(std::ostream& stream, const RSARArguments& arguments) {
    stream << "c RSAR arguments: " << std::endl << "c   RSAR enabled: " << arguments.useRSAR << std::endl;
    if (arguments.useRSAR) {
        stream << "c   Max. refinement steps: " << arguments.maxRefinementSteps << std::endl
        << "c   Simplification handling mode: " << static_cast<int>(arguments.simplificationHandlingMode) << std::endl
        << "c   Use input dependency size heuristic: " << arguments.withInputDepCountHeuristic << std::endl
        << "c   Input dependency size heuristic configuration: " << arguments.inputDepCountHeuristicConfiguration << std::endl
        << "c   Min. gate count: " << arguments.minGateCount << std::endl;
    }
    
    return stream;
}

static Candy::SimplificationHandlingMode parseSimplificationHandlingMode(const std::string& str) {
    if (str == "DISABLE") {
        return Candy::SimplificationHandlingMode::DISABLE;
    }
    if (str == "FREEZE") {
        return Candy::SimplificationHandlingMode::FREEZE;
    }
    if (str == "RESTRICT") {
        return Candy::SimplificationHandlingMode::RESTRICT;
    }
    if (str == "FULL") {
        return Candy::SimplificationHandlingMode::FULL;
    }
    throw std::invalid_argument(str + ": Unknown simplification handling mode");
}


static std::unique_ptr<Candy::Conjectures> performRandomSimulation(Candy::GateAnalyzer &analyzer,
                                                                   const RandomSimulationArguments& rsArguments,
                                                                   std::chrono::milliseconds timeLimit = std::chrono::milliseconds{-1}) {
    auto simulatorBuilder = Candy::createDefaultRandomSimulatorBuilder();
    simulatorBuilder->withGateAnalyzer(analyzer);

    if (rsArguments.abortByRRAT) {
        simulatorBuilder->withReductionRateAbortThreshold(rsArguments.rrat);
    }

    if (rsArguments.filterGatesByNonmono) {
        simulatorBuilder->withGateFilter(Candy::createNonmonotonousGateFilter(analyzer));
    }

    auto randomSimulator = simulatorBuilder->build();
    // TODO: time limit
    auto conjectures = randomSimulator->run(static_cast<unsigned int>(rsArguments.nRounds), timeLimit);

    if (rsArguments.filterConjecturesBySize) {
        auto sizeFilter = Candy::createSizeConjectureFilter(rsArguments.maxConjectureSize);
        conjectures = sizeFilter->apply(conjectures);
    }

    if (rsArguments.removeBackboneConjectures) {
        auto bbFilter = Candy::createBackboneRemovalConjectureFilter();
        conjectures = bbFilter->apply(conjectures);
    }

    return backported_std::make_unique<Candy::Conjectures>(std::move(conjectures));
}

std::vector<size_t> getARInputDepCountHeuristicLimits(const std::string& limitsString) {
    
    /* TODO: user input validation is currently broken.
    
    std::regex unsignedIntRegex { "^(\\s*[0-9]+\\s*)+$" };
    if (!std::regex_match(limitsString, unsignedIntRegex)) {
        throw std::invalid_argument(limitsString + ": invalid limits");
    }
    */

    std::vector<size_t> limits = Candy::tokenizeByWhitespace<size_t>(limitsString);

    if (limits.size() == 0) {
        throw std::invalid_argument(limitsString + ": invalid limits");
    }

    return limits;
}

std::unique_ptr<Candy::ARSolver> createARSolver(Candy::GateAnalyzer& analyzer, DefaultSimpSolver& satSolver, std::unique_ptr<Candy::Conjectures> conjectures,
                const RSARArguments& rsarArguments) {
    auto arSolverBuilder = Candy::createARSolverBuilder();
    arSolverBuilder->withConjectures(std::move(conjectures));
    arSolverBuilder->withMaxRefinementSteps(rsarArguments.maxRefinementSteps);
    arSolverBuilder->withSimplificationHandlingMode(rsarArguments.simplificationHandlingMode);
    arSolverBuilder->withSolver(Candy::createNonowningGlucoseAdapter(satSolver));

    if (rsarArguments.withInputDepCountHeuristic) {
        auto limits = getARInputDepCountHeuristicLimits(rsarArguments.inputDepCountHeuristicConfiguration);
        arSolverBuilder->addRefinementHeuristic(Candy::createInputDepCountRefinementHeuristic(analyzer, limits));
    }

    return arSolverBuilder->build();
}


template<class SolverType> static
typename std::enable_if<std::is_same<SolverType, DefaultSimpSolver>::value, lbool>::type
solveWithRSAR(SolverType& solver, Candy::CNFProblem& problem, const GateRecognitionArguments& gateRecognitionArgs,
                const RandomSimulationArguments& rsArguments, const RSARArguments& rsarArguments) {
    // TODO: the CPU time code was inserted in quite a hurry and
    // needs to be refactored.
    
    std::chrono::milliseconds startCPUTime = Glucose::cpuTime();
    
    GateRecognitionArguments localGateRecognitionArgs = gateRecognitionArgs;
    if (rsArguments.preprocessingTimeLimit >= std::chrono::milliseconds{0}) {
        localGateRecognitionArgs.opt_gr_timeout = std::min(gateRecognitionArgs.opt_gr_timeout,
                                                           rsArguments.preprocessingTimeLimit);
    }

    auto gateAnalyzer = createGateAnalyzer(problem, localGateRecognitionArgs);
    gateAnalyzer->analyze();
    
    std::chrono::milliseconds gateAnalyzerTime = Glucose::cpuTime() - startCPUTime;
    std::cerr << "c Gate recognition time: " << gateAnalyzerTime.count() << " ms" << std::endl;
    
    try {
        if (gateAnalyzer->hasTimeout()
            || (rsArguments.preprocessingTimeLimit >= std::chrono::milliseconds{0}
                && gateAnalyzerTime > rsArguments.preprocessingTimeLimit)) {
            throw UnsuitableProblemException{"Gate recognition exceeded the time limit."};
        }
        
        
        if (gateAnalyzer->getGateCount() < rsarArguments.minGateCount) {
            throw UnsuitableProblemException{std::string{"Insufficient gate count "}
                + std::to_string(gateAnalyzer->getGateCount())};
        }
        
        std::chrono::milliseconds rsTimeLimit = std::chrono::milliseconds{-1};
        if (rsArguments.preprocessingTimeLimit >= std::chrono::milliseconds{0}) {
            rsTimeLimit = rsArguments.preprocessingTimeLimit - gateAnalyzerTime;
        }
        
        try {
            auto conjectures = performRandomSimulation(*gateAnalyzer, rsArguments, rsTimeLimit);
            if (conjectures->getEquivalences().empty() && conjectures->getBackbones().empty()) {
                throw UnsuitableProblemException{"No conjectures found."};
            }
            
            auto randomSimulationTime = Glucose::cpuTime() - startCPUTime - gateAnalyzerTime;
            std::cerr << "c Random simulation time: " << randomSimulationTime.count() << " ms" << std::endl;
            
    
            auto arSolver = createARSolver(*gateAnalyzer, solver, std::move(conjectures), rsarArguments);
            auto result = arSolver->solve();

            return lbool(result);
        }
        catch (OutOfTimeException& e) {
            auto randomSimulationTime = Glucose::cpuTime() - startCPUTime - gateAnalyzerTime;
            std::cerr << "c Random simulation time: " << randomSimulationTime.count() << " ms" << std::endl;
            
            throw UnsuitableProblemException{"Random simulation exceeded the time limit."};
        }
    }
    catch (UnsuitableProblemException& e) {
        std::cerr << "c Aborting RSAR: " << e.what() << std::endl;
        std::cerr << "c Falling back to unmodified Candy." << std::endl;
        gateAnalyzer.reset();
        return solve(solver, true); // TODO: use the do_preprocess parameter here.
    }
}

template<class SolverType> static
typename std::enable_if<!std::is_same<SolverType, DefaultSimpSolver>::value, lbool>::type
solveWithRSAR(SolverType& solver, Candy::CNFProblem& problem, const GateRecognitionArguments& gateRecognitionArgs,
              const RandomSimulationArguments& rsArguments, const RSARArguments& rsarArguments) {
    throw std::logic_error("solveWithRSAR may only be called with SolverType == DefaultSimpSolver");
}



enum class RSILMode {
    UNRESTRICTED,
    VANISHING,
    IMPLICATIONBUDGETED
};

struct RSILArguments {
    const bool useRSIL;
    const RSILMode mode;
    const uint64_t vanishing_probabilityHalfLife;
    const uint64_t impbudget_initialBudget;
    
    const bool filterByInputDependencies;
    const int filterByInputDependenciesMax;
    const bool filterOnlyBackbones;
    const double minGateOutputFraction;

    const bool useRSILOnlyForMiters;
};

std::ostream& operator <<(std::ostream& stream, const RSILArguments& arguments) {
    stream << "c RSIL arguments: " << std::endl << "c   RSIL enabled: " << arguments.useRSIL << std::endl;
    if (arguments.useRSIL) {
        stream << "c   RSIL mode: " << static_cast<int>(arguments.mode) << std::endl
        << "c   Vanishing mode half-life: " << arguments.vanishing_probabilityHalfLife << std::endl
        << "c   Implication budget mode initial budgets: " << arguments.impbudget_initialBudget << std::endl
        << "c   Filter by input dependency count enabled: " << arguments.filterByInputDependencies << std::endl
        << "c   Max. input depdencency count: " << arguments.filterByInputDependenciesMax << std::endl
        << "c   Apply filters only to backbone conjectures: " << arguments.filterOnlyBackbones << std::endl
        << "c   Min. gate output fraction: " << arguments.minGateOutputFraction << std::endl
        << "c   RSIL restricted to miters?: " << arguments.useRSILOnlyForMiters << std::endl;
    }
    
    return stream;
}

static RSILMode getRSILMode(const std::string& mode) {
    if (mode == "unrestricted") {
        return RSILMode::UNRESTRICTED;
    }
    else if (mode == "vanishing") {
        return RSILMode::VANISHING;
    }
    else if (mode == "implicationbudgeted") {
        return RSILMode::IMPLICATIONBUDGETED;
    }
    else {
        throw std::invalid_argument("Error: unknown RSIL mode " + mode);
    }
}

using RSILSolver = SimpSolver<RSILBranchingHeuristic2>;
using RSILVanishingSolver = SimpSolver<RSILVanishingBranchingHeuristic2>;
using RSILBugetSolver = SimpSolver<RSILBudgetBranchingHeuristic2>;


// getRSILHeuristicParameters is implemented using SFINAE.
//
// Solver<PickBranchLitType> exports the PickBranchLitType template argument as
// Solver::PickBranchLitType, which in case of RSIL has a nested type
// PickBranchLitType::BasicType (making life easier due to further template
// arguments, such as the advice entry type. For each RSIL branching heuristic
// type HEUR<T>, there is a type HEUR<T>::BasicType which is independent of T.
// For example, we have
// RSILBudgetBranchingHeuristic<3>::BasicType == RSILBudgetBranchingHeuristic<300>::BasicType
//
// As getRSILHeuristicParameters is independent of the heuristic template parameters,
// getRSILHeuristicParameters is effectively specialized for the BasicType subtypes.
// This is achieved via a common SFINAE pattern using std::enable_if and std::is_same:
// if the std::is_same expression holds (i.e. std::is_same<...>::value is true), the
// type std::enable_if<std::is_same<...>::value, T>::type is T, and otherwise
// std::enable_if<std::is_same<...>::value, T>::type does not exist. (In this
// case, T is the function's return value type.) The compiler uses the specialization
// where std::enable_if<...>::type is defined.

template<class PickBranchLitType> static
typename std::enable_if<std::is_same<typename PickBranchLitType::BasicType, RSILBranchingHeuristic3::BasicType>::value,
typename PickBranchLitType::Parameters>::type
getRSILHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer) {
    
    bool useBackbones = !conjectures.getBackbones().empty();
    std::shared_ptr<RefinementHeuristic> filterHeuristic = nullptr;
    
    if (rsilArgs.filterByInputDependencies) {
        auto maxInputs = static_cast<unsigned long>(rsilArgs.filterByInputDependenciesMax);
        auto heuristic = createInputDepCountRefinementHeuristic(analyzer, {maxInputs, 0});
        heuristic->beginRefinementStep();
        filterHeuristic = shared_ptr<RefinementHeuristic>(heuristic.release());
    }
    
    return typename PickBranchLitType::Parameters{conjectures,
                                                  useBackbones,
                                                  rsilArgs.filterByInputDependencies,
                                                  filterHeuristic,
                                                  rsilArgs.filterOnlyBackbones};
}

template<class PickBranchLitType> static
typename std::enable_if<std::is_same<typename PickBranchLitType::BasicType, RSILVanishingBranchingHeuristic3::BasicType>::value,
typename PickBranchLitType::Parameters>::type
getRSILHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer) {
    auto conf = getRSILHeuristicParameters<typename PickBranchLitType::UnderlyingHeuristicType>(conjectures,
                                                                                                rsilArgs,
                                                                                                analyzer);
    return {conf, rsilArgs.vanishing_probabilityHalfLife};
}

template<class PickBranchLitType> static
typename std::enable_if<std::is_same<typename PickBranchLitType::BasicType, RSILBudgetBranchingHeuristic3::BasicType>::value,
typename PickBranchLitType::Parameters>::type
getRSILHeuristicParameters(const Conjectures& conjectures, const RSILArguments& rsilArgs, GateAnalyzer& analyzer) {
    auto conf = getRSILHeuristicParameters<typename PickBranchLitType::UnderlyingHeuristicType>(conjectures,
                                                                                                rsilArgs,
                                                                                                analyzer);
    return {conf, rsilArgs.impbudget_initialBudget};
}


template<class SolverType> static
std::function<void(SolverType&, CNFProblem&)>
createRSILPreprocessingHook(const GateRecognitionArguments& gateRecognitionArgs,
                            const RandomSimulationArguments& randomSimulationArgs,
                            const RSILArguments& rsilArgs) {
    return [gateRecognitionArgs, randomSimulationArgs, rsilArgs](SolverType& solver, CNFProblem& problem) {
        Runtime runtime;
        GateRecognitionArguments localGateRecognitionArgs = gateRecognitionArgs;
        if (randomSimulationArgs.preprocessingTimeLimit >= std::chrono::milliseconds{-1}) {
            runtime.setTimeout(randomSimulationArgs.preprocessingTimeLimit);
            localGateRecognitionArgs.opt_gr_timeout = std::min(gateRecognitionArgs.opt_gr_timeout,
                                                               randomSimulationArgs.preprocessingTimeLimit);
        }
        runtime.start();
        
        auto analyzer = createGateAnalyzer(problem, localGateRecognitionArgs);
        analyzer->analyze();
        std::cerr << "c Gate recognition time: " << runtime.lap().count() << " ms" << std::endl;
        
        double problemVars = problem.nVars();
        double gateOutputs = analyzer->getGateCount();
        
        if (analyzer->hasTimeout() || runtime.hasTimeout()) {
            // Abort RSIL, since the probability is too high that this run
            // is not reproducible if we continue
            throw UnsuitableProblemException{"gate analysis exceeded the preprocessing time limit."};
        }
        
        if (problemVars == 0 || (gateOutputs/problemVars) < rsilArgs.minGateOutputFraction) {
            std::string errorMessage = std::string{"insufficient gate count "} + std::to_string(analyzer->getGateCount())
            + std::string{"/"} + std::to_string(problem.nVars()) + std::string{"."};
            throw UnsuitableProblemException{errorMessage};
        }
        
        if (rsilArgs.useRSILOnlyForMiters) {
            bool isMiter = hasPossiblyMiterStructure(*analyzer);
            std::cerr << "c Miter recognition time: " << runtime.lap().count() << " ms" << std::endl;
            
            if (runtime.hasTimeout()) {
                throw UnsuitableProblemException{"miter detection exceeded the preprocessing time limit."};
            }
            if (!isMiter) {
                throw UnsuitableProblemException{"problem heuristically determined not to be a miter problem."};
            }
        }

        std::unique_ptr<Conjectures> conjectures;
        
        if (randomSimulationArgs.preprocessingTimeLimit >= std::chrono::milliseconds{0}) {
            auto remainingTime = randomSimulationArgs.preprocessingTimeLimit - runtime.getRuntime();
            try {
                conjectures = performRandomSimulation(*analyzer, randomSimulationArgs, remainingTime);
            }
            catch(OutOfTimeException& exception) {
                std::cerr << "c Random simulation time: " << runtime.lap().count() << " ms" << std::endl;
                throw UnsuitableProblemException{"random simulation exceeded the preprocessing time limit."};
            }
        }
        else {
            conjectures = performRandomSimulation(*analyzer, randomSimulationArgs);
        }
        
        std::cerr << "c Random simulation time: " << runtime.lap().count() << " ms" << std::endl;
        
        if (conjectures->getEquivalences().empty() || conjectures->getBackbones().empty()) {
            throw UnsuitableProblemException{"no conjectures found."};
        }
        
        auto heuristicParameters = getRSILHeuristicParameters<typename SolverType::PickBranchLitType>(*conjectures,
                                                                                                      rsilArgs,
                                                                                                      *analyzer);
        solver.initializePickBranchLit(heuristicParameters);
    };
}


struct GlucoseArguments {
    const int verb;
    const bool mod;
    const int vv;

    const int cpu_lim;
    const int mem_lim;

    const bool do_solve;
    const bool do_preprocess;
    const bool do_certified;
    const bool do_gaterecognition;
    const bool do_simp_out;

    const char *opt_certified_file;

    const bool wait_for_user;
    
    const bool read_from_stdin;
    const char *input_filename;
    const char *output_filename;

    const GateRecognitionArguments gateRecognitionArgs;
    const RandomSimulationArguments randomSimulationArgs;
    const RSARArguments rsarArgs;
    const RSILArguments rsilArgs;
};

std::ostream& operator <<(std::ostream& stream, const GlucoseArguments& arguments) {
    stream << "c Glucose arguments: " << std::endl
    << "c   Verbosity: " << arguments.verb << std::endl
    << "c   Show model: " << arguments.mod << std::endl
    << "c   Verboisty every conflicts: " << arguments.vv << std::endl
    << "c   CPU time limit: " << arguments.cpu_lim << std::endl
    << "c   Memory limit: " << arguments.mem_lim << std::endl
    << "c   Solve: " << arguments.do_solve << std::endl
    << "c   Preprocess: " << arguments.do_preprocess << std::endl
    << "c   Certified UNSAT: " << arguments.do_certified << std::endl
    << "c   Output Simplified Dimacs: " << arguments.do_simp_out << std::endl
    << "c   Benchmark gate recognition: " << arguments.do_gaterecognition << std::endl
    << "c   Certified UNSAT output: " << (arguments.opt_certified_file == nullptr ?
                                        "(none)" : arguments.opt_certified_file) << std::endl
    << "c   Wait for user: " << arguments.wait_for_user << std::endl
    << "c   Read problem from stdin: " << arguments.read_from_stdin << std::endl
    << "c   Input filename: " << (arguments.input_filename == nullptr ?
                                "(none)" : arguments.input_filename) << std::endl
    << "c   Output filename: "<< (arguments.output_filename == nullptr ?
                                "(none)" : arguments.output_filename) << std::endl;
    
    stream << arguments.gateRecognitionArgs;
    stream << arguments.randomSimulationArgs;
    stream << arguments.rsarArgs;
    stream << arguments.rsilArgs;
    
    return stream;
}






static GlucoseArguments parseCommandLineArgs(int& argc, char** argv) {
    using namespace Glucose;
    setUsageHelp("c USAGE: %s [options] <input-file> <result-output-file>\n\n  where input may be either in plain or gzipped DIMACS.\n");

// Extra options:
//
    IntOption verb("MAIN", "verb", "Verbosity level (0=silent, 1=some, 2=more).", 1, IntRange(0, 2));
    BoolOption mod("MAIN", "model", "show model.", false);
    IntOption vv("MAIN", "vv", "Verbosity every vv conflicts", 10000, IntRange(1, INT32_MAX));

    IntOption cpu_lim("MAIN", "cpu-lim", "Limit on CPU time allowed in seconds.\n", INT32_MAX, IntRange(0, INT32_MAX));
    IntOption mem_lim("MAIN", "mem-lim", "Limit on memory usage in megabytes.\n", INT32_MAX, IntRange(0, INT32_MAX));
    BoolOption wait_for_user("MAIN", "wait", "Wait for user input on startup (for profiling).", false);

    BoolOption do_solve("METHOD", "solve", "Completely turn on/off actual sat solving.", true);
    BoolOption do_preprocess("METHOD", "pre", "Completely turn on/off any preprocessing.", true);
    BoolOption do_certified("METHOD", "certified", "Certified UNSAT using DRUP format", false);
    BoolOption do_gaterecognition("METHOD", "gates", "Completely turn on/off actual gate recognition.", false);
    BoolOption do_simp_out("METHOD", "simp-out", "Simplify only and output dimacs.", false);

    StringOption opt_certified_file("CERTIFIED UNSAT", "certified-output", "Certified UNSAT output file", "NULL");
    
    BoolOption opt_print_gates("GATE RECOGNITION", "print-gates", "print gates.", false);
    IntOption opt_gr_tries("GATE RECOGNITION", "gate-tries", "Number of heuristic clause selections to enter recursion", 0, IntRange(0, INT32_MAX));
    BoolOption opt_gr_patterns("GATE RECOGNITION", "gate-patterns", "Enable Pattern-based Gate Detection", true);
    BoolOption opt_gr_semantic("GATE RECOGNITION", "gate-semantic", "Enable Semantic Gate Detection", true);
    IntOption opt_gr_semantic_budget("GATE RECOGNITION", "gate-semantic-budget", "Enable Semantic Gate Detection Conflict Budget", 0, IntRange(0, INT32_MAX));
    IntOption opt_gr_timeout("GATE RECOGNITION", "gate-timeout", "Enable Gate Detection Timeout (seconds)", 0, IntRange(0, INT32_MAX));
    BoolOption opt_gr_holistic("GATE RECOGNITION", "gate-holistic", "Enable Holistic Gate Detection", false);
    BoolOption opt_gr_lookahead("GATE RECOGNITION", "gate-lookahead", "Enable Local Blocked Elimination", false);
    IntOption opt_gr_lookahead_threshold("GATE RECOGNITION", "gate-lookahead-threshold", "Local Blocked Elimination Threshold", 10, IntRange(1, INT32_MAX));
    BoolOption opt_gr_intensify("GATE RECOGNITION", "gate-intensification", "Enable Intensification", true);

    IntOption opt_rs_nrounds("RANDOMSIMULATION", "rs-rounds", "Amount of random simulation rounds (gets rounded up to the next multiple of 2048)", 1048576,
                    IntRange(1, INT32_MAX));
    BoolOption opt_rs_abortbyrrat("RANDOMSIMULATION", "rs-abort-by-rrat", "Abort random simulation when the reduction rate falls below the RRAT threshold",
                    false);
    DoubleOption opt_rs_rrat("RANDOMSIMULATION", "rs-rrat", "Reduction rate abort threshold", 0.01, DoubleRange(0.0, true, 1.0, false));
    IntOption opt_rs_filterConjBySize("RANDOMSIMULATION", "rs-max-conj-size", "Max. allowed literal equivalence conjecture size (0: disable filtering by size)",
                    0, IntRange(0, INT32_MAX));
    BoolOption opt_rs_removeBackboneConj("RANDOMSIMULATION", "rs-remove-backbone-conj", "Filter out conjectures about the problem's backbone", false);
    BoolOption opt_rs_filterGatesByNonmono("RANDOMSIMULATION", "rs-only-nonmono-gates", "Use only nonmonotonously nested gates for random simulation", false);
    IntOption opt_rs_ppTimeLimit("RANDOMSIMULATION", "rs-time-limit", "Time limit for preprocessing (gate recognition + rs; -1 to disable; default: disabled)", -1, IntRange(-1, INT32_MAX));

    BoolOption opt_rsar_enable("RSAR", "rsar-enable", "Enable random-simulation-based abstraction refinement SAT solving", false);
    IntOption opt_rsar_maxRefinementSteps("RSAR", "rsar-max-refinements", "Max. refinement steps", 10, IntRange(1, INT32_MAX));
    StringOption opt_rsar_simpMode("RSAR", "rsar-simpmode", "Simplification handling mode", "RESTRICT");
    StringOption opt_rsar_inputDepCountHeurConf("RSAR", "rsar-heur-idc", "Input dependency count heuristic configuration", "");
    IntOption opt_rsar_minGateCount("RSAR", "rsar-min-gatecount", "Minimum amount of recognized gates for RSAR to be enabled",
                                    100, IntRange(1, std::numeric_limits<int>::max()));
    
    BoolOption opt_rsil_enable("RSIL", "rsil-enable", "Enable random-simulation-based implicit learning heuristics", false);
    StringOption opt_rsil_mode("RSIL", "rsil-mode", "Set RSIL mode to unrestricted, vanishing or implicationbudgeted", "unrestricted");
    IntOption opt_rsil_vanHalfLife("RSIL", "rsil-van-halflife", "Set the probability half-life (in decisions) for vanishing mode",
                                   1 << 24, IntRange(1, INT32_MAX));
    IntOption opt_rsil_impBudgets("RSIL", "rsil-imp-budgets", "Set the initial budgets for implicationbudgeted mode",
                                  1 << 20, IntRange(1, INT32_MAX));
    
    IntOption opt_rsil_filterByInputDeps("RSIL", "rsil-filter-by-input-dependencies",
                                         "Disregard variables dependending on more than N inputs. N=0 (default) disables this filter.",
                                         0,
                                         IntRange(0, INT32_MAX));
    BoolOption opt_rsil_filterOnlyBackbone("RSIL", "rsil-filter-only-backbone",
                                           "Filter only the backbone of the problem via rsil-filter-by-input-dependencies",
                                           false);
    DoubleOption opt_rsil_minGateFraction("RSIL", "rsil-min-gate-frac", "Enable RSIL only when at least this fraction of the variables"\
                                          " consists of gate outputs", 0.1, DoubleRange(0.0, true, 1.0, true));
    BoolOption opt_rsil_onlyMiters("RSIL", "rsil-only-miters", "Enable RSIL only for miter problems (heuristic detection)", false);

    parseOptions(argc, argv, true);

    GateRecognitionArguments gateRecognitionArgs{
        opt_gr_tries,
        opt_gr_patterns,
        opt_gr_semantic,
        static_cast<unsigned int>(opt_gr_semantic_budget),
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds{opt_gr_timeout}),
        opt_gr_holistic,
        opt_gr_lookahead,
        opt_gr_intensify,
        opt_gr_lookahead_threshold,
        opt_print_gates
    };

    RandomSimulationArguments rsArgs{
        opt_rs_nrounds,
        opt_rs_abortbyrrat,
        opt_rs_rrat,
        opt_rs_filterConjBySize > 0,
        opt_rs_filterConjBySize,
        opt_rs_removeBackboneConj,
        opt_rs_filterGatesByNonmono,
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds{opt_rs_ppTimeLimit})
    };

    RSARArguments rsarArgs{
        opt_rsar_enable,
        opt_rsar_maxRefinementSteps,
        parseSimplificationHandlingMode(std::string{opt_rsar_simpMode}),
        std::string{opt_rsar_inputDepCountHeurConf} != "",
        std::string{opt_rsar_inputDepCountHeurConf},
        opt_rsar_minGateCount
    };
    
    RSILArguments rsilArgs{
        opt_rsil_enable,
        getRSILMode(std::string{opt_rsil_mode}),
        static_cast<uint64_t>(opt_rsil_vanHalfLife),
        static_cast<uint64_t>(opt_rsil_impBudgets),
        opt_rsil_filterByInputDeps != 0,
        opt_rsil_filterByInputDeps,
        opt_rsil_filterOnlyBackbone,
        opt_rsil_minGateFraction,
        opt_rsil_onlyMiters
    };

    const char* outputFilename = (argc >= 3) ? argv[argc - 1] : nullptr;
    bool readFromStdIn = (argc == 1);
    const char* inputFilename = (!readFromStdIn ? argv[1] : nullptr);
    
    return GlucoseArguments{
        verb,
        mod,
        vv,
        cpu_lim,
        mem_lim,
        do_solve,
        do_preprocess,
        do_certified,
        do_gaterecognition,
        do_simp_out,
        opt_certified_file,
        wait_for_user,
        readFromStdIn,
        inputFilename,
        outputFilename,
        gateRecognitionArgs,
        rsArgs,
        rsarArgs,
        rsilArgs
    };
}

static void waitForUserInput() {
    std::cout << "Press enter to continue." << std::endl;
    std::getchar();
}


/**
 * Run Propagate and Simplify, then output the simplified CNF Problem
 */
template<class SolverType>
static lbool simplifyAndPrintProblem(SolverType& S) {
    lbool result = l_Undef;

    S.setPropBudget(1);

    vector<Lit> assumptions;
    result = S.solve(assumptions);
    S.simplify();
    S.printDIMACS();

    return result;
}


template<class SolverType = DefaultSimpSolver>
int executeSolver(const GlucoseArguments& args,
                  SolverType& S,
                  CNFProblem& problem) {
    Statistics::getInstance().runtimeStart(RT_INITIALIZATION);
    S.addClauses(problem);
    Statistics::getInstance().runtimeStop(RT_INITIALIZATION);
    
    if (S.verbosity > 0) {
        printProblemStatistics(S);
        Statistics::getInstance().printRuntime(RT_INITIALIZATION);
        printf("c |                                                                                            |\n");
    }
    
    // Change to signal-handlers that will only notify the solver and allow it to terminate voluntarily
    installSignalHandlers(true, &S);
    
    lbool result;
    if (args.do_simp_out) {
        result = simplifyAndPrintProblem(S);
    } else if (!args.rsarArgs.useRSAR) {
        result = solve(S, args.do_preprocess);
    } else {
        result = solveWithRSAR(S, problem, args.gateRecognitionArgs, args.randomSimulationArgs, args.rsarArgs);
    }
    
    if (!args.do_simp_out) {
        const char* statsFilename = args.output_filename;
        printResult(S, result, args.mod, statsFilename);
    }
    
    exit((result == l_True ? 10 : result == l_False ? 20 : 0));
    return (result == l_True ? 10 : result == l_False ? 20 : 0);
}

template<class SolverType = DefaultSimpSolver>
int solve(const GlucoseArguments& args,
          std::function<void(SolverType&, CNFProblem&)> preprocessingHook = {}) {
    try {
        // Use signal handlers that forcibly quit until the solver will be able to respond to interrupts:
        installSignalHandlers<SolverType>(false, nullptr);

        setLimits(args.cpu_lim, args.mem_lim);

        Statistics::getInstance().runtimeStart(RT_INITIALIZATION);

        std::unique_ptr<Certificate> certificate = backported_std::make_unique<Certificate>(args.opt_certified_file,
                                                                                            args.do_certified);

        Candy::CNFProblem problem(*certificate);
        if (args.read_from_stdin) {
            printf("c Reading from standard input... Use '--help' for help.\n");
            if (!problem.readDimacsFromStdout()) {
                return 1;
            }
        } else {
            if (!problem.readDimacsFromFile(args.input_filename)) {
                return 1;
            }
        }

        auto S = backported_std::make_unique<SolverType>();
        S->setVerbosities(args.vv, args.verb);
        S->setCertificate(*certificate);
        
        bool fallBackToUnmodifiedCandy = false;
        std::unique_ptr<DefaultSimpSolver> fallbackSolver{};
        
        if (preprocessingHook) {
            try {
                preprocessingHook(*S, problem);
            }
            catch(UnsuitableProblemException& e) {
                std::cerr << "c Aborting RSIL: " << e.what() << std::endl;
                std::cerr << "c Falling back to unmodified Candy" << std::endl;
                fallBackToUnmodifiedCandy = true;
                S.reset(nullptr);
                fallbackSolver = backported_std::make_unique<DefaultSimpSolver>();
                fallbackSolver->setVerbosities(args.vv, args.verb);
                fallbackSolver->setCertificate(*certificate);
            }
        }

        Statistics::getInstance().runtimeStop(RT_INITIALIZATION);

        if (args.do_gaterecognition) {
            benchmarkGateRecognition(problem, args.gateRecognitionArgs);
            return 0;
        }
        
        if (!fallBackToUnmodifiedCandy) {
            return executeSolver(args, *S, problem);
        }
        else {
            return executeSolver(args, *fallbackSolver, problem);
        }
        
    } catch (std::bad_alloc& ba) {
        //printf("c Bad_Alloc Caught: %s\n", ba.what());
        printf("c ==============================================================================================\n");
        printf("s INDETERMINATE\n");
        return 0;
    }
}

static int solveWithRSIL(const GlucoseArguments& args) {
    if(args.rsilArgs.mode == RSILMode::UNRESTRICTED) {
        auto preprocessor = createRSILPreprocessingHook<RSILSolver>(args.gateRecognitionArgs,
                                                                    args.randomSimulationArgs,
                                                                    args.rsilArgs);
        return solve<RSILSolver>(args, preprocessor);
    }
    
    if (args.rsilArgs.mode == RSILMode::VANISHING) {
        auto preprocessor = createRSILPreprocessingHook<RSILVanishingSolver>(args.gateRecognitionArgs,
                                                                             args.randomSimulationArgs,
                                                                             args.rsilArgs);
        return solve<RSILVanishingSolver>(args, preprocessor);
    }
    
    if (args.rsilArgs.mode == RSILMode::IMPLICATIONBUDGETED) {
        auto preprocessor = createRSILPreprocessingHook<RSILBugetSolver>(args.gateRecognitionArgs,
                                                                         args.randomSimulationArgs,
                                                                         args.rsilArgs);
        return solve<RSILBugetSolver>(args, preprocessor);
    }
    
    throw std::invalid_argument{"RSIL mode not implemented"};
}

//=================================================================================================
// Main:
int main(int argc, char** argv) {
    std::cout << "c Candy 0.3 is made of Glucose (Many thanks to the Glucose and MiniSAT teams)" << std::endl;
    
    GlucoseArguments args = parseCommandLineArgs(argc, argv);
    
    if (args.verb > 0) {
        std::cout << args << std::endl;
    }

    if (args.rsilArgs.useRSIL && args.rsarArgs.useRSAR) {
        throw std::invalid_argument("Usign RSAR with RSIL is not yet supported");
    }
    
    if (args.wait_for_user) {
        waitForUserInput();
    }
    
    if (args.rsilArgs.useRSIL) {
        solveWithRSIL(args);
    }
    else {
        return solve<>(args);
    }
}

