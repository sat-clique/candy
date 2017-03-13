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
#include <sys/resource.h>
#include <string>
#include <stdexcept>
#include <regex>

#include "candy/core/CNFProblem.h"
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
#include "candy/rsar/ARSolver.h"
#include "candy/rsar/SolverAdapter.h"
#include "candy/rsar/Heuristics.h"
#include "candy/randomsimulation/RandomSimulator.h"
#include "candy/randomsimulation/Conjectures.h"
#include "candy/randomsimulation/ClauseOrder.h"
#include "candy/randomsimulation/SimulationVector.h"

using namespace Candy;

static Solver* solver;

// Terminate by notifying the solver and back out gracefully. This is mainly to have a test-case
// for this feature of the Solver as it may take longer than an immediate call to '_exit()'.
static void SIGINT_interrupt(int signum) {
    solver->setInterrupt(true);
}

// Note that '_exit()' rather than 'exit()' has to be used. The reason is that 'exit()' calls
// destructors and may cause deadlocks if a malloc/free function happens to be running (these
// functions are guarded by locks for multithreaded use).
static void SIGINT_exit(int signum) {
    printf("\n*** INTERRUPTED ***\n");
    if (solver->verbosity > 0) {
        double cpu_time = Glucose::cpuTime();
        double mem_used = 0; //memUsedPeak();
        Statistics::getInstance().printFinalStats(cpu_time, mem_used, solver->nConflicts, solver->nPropagations);
    }
    _exit(1);
}

static void setLimits(int cpu_lim, int mem_lim) {
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
}

static void printModel(FILE* f, Solver* solver) {
    fprintf(f, "v");
    for (int i = 0; i < solver->nVars(); i++)
        if (solver->model[i] != l_Undef)
            fprintf(f, " %s%d", (solver->model[i] == l_True) ? "" : "-", i + 1);
    fprintf(f, " 0\n");
}

/**
 * Runs the SAT solver, performing simplification if \p do_preprocess is true.
 */
static lbool solve(SimpSolver& S, bool do_preprocess, double parsed_time) {
    lbool result = l_Undef;

    if (do_preprocess/* && !S.isIncremental()*/) {
        printf("c | Preprocesing is fully done\n");
        S.eliminate(true);
        if (!S.okay())
            result = l_False;
        double simplified_time = Glucose::cpuTime();
        if (S.verbosity > 0) {
            printf("c |  Simplification time:  %12.2f s                                                                 |\n", simplified_time - parsed_time);
            if (result == l_False) {
                printf("c =========================================================================================================\n");
                printf("Solved by simplification\n");
            }
        }
    }
    printf("c |                                                                                                       |\n");

    if (result == l_Undef) {
        vector<Lit> assumptions;
        result = S.solveLimited(assumptions);
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
static void printResult(Solver& S, lbool result, const char* outputFilename = nullptr) {
    if (S.verbosity > 0) {
        double cpu_time = Glucose::cpuTime();
        double mem_used = 0; //memUsedPeak();
        Statistics::getInstance().printFinalStats(cpu_time, mem_used, solver->nConflicts, solver->nPropagations);
        Statistics::getInstance().printAllocatorStatistics();
    }

    printf(result == l_True ? "s SATISFIABLE\n" : result == l_False ? "s UNSATISFIABLE\n" : "s INDETERMINATE\n");

    FILE* res = outputFilename != nullptr ? fopen(outputFilename, "wb") : nullptr;
    if (res != NULL) {
        fprintf(res, result == l_True ? "s SATISFIABLE\n" : result == l_False ? "s UNSATISFIABLE\n" : "s INDETERMINATE\n");
        if (result == l_True) {
            printModel(res, solver);
        }
        fclose(res);
    } else {
        if (S.showModel && result == l_True) {
            printModel(stdout, solver);
        }
    }
}

/**
 * Installs signal handlers for SIGINT and SIGXCPU.
 * If \p handleInterruptsBySolver is true, the interrupts are handled by SIGINT_interrupt();
 * otherwise, they are set up to be handled by SIGINT_exit().
 */
static void installSignalHandlers(bool handleInterruptsBySolver) {
    if (handleInterruptsBySolver) {
        signal(SIGINT, SIGINT_interrupt);
        signal(SIGXCPU, SIGINT_interrupt);
    } else {
        signal(SIGINT, SIGINT_exit);
        signal(SIGXCPU, SIGINT_exit);
    }
}

/**
 * Prints statistics about the problem to be solved.
 */
static void printProblemStatistics(Solver& S, double parseTime) {
    printf("c ========================================[ Problem Statistics ]===========================================\n");
    printf("c |                                                                                                       |\n");
    printf("c |  Number of variables:  %12d                                                                   |\n", S.nVars());
    printf("c |  Number of clauses:    %12d                                                                   |\n", S.nClauses());
    printf("c |  Parse time:           %12.2f s                                                                 |\n", parseTime);
    printf("c |                                                                                                       |\n");
}

/**
 * Configures the SAT solver \p S.
 * 
 * TODO: document parameters
 */
static void configureSolver(SimpSolver& S, int verbosity, int verbosityEveryConflicts, bool showModel, bool certifiedAllClauses, bool certifiedUNSAT,
                const char* certifiedUNSATfile) {
    S.verbosity = verbosity;
    S.verbEveryConflicts = verbosityEveryConflicts;
    S.showModel = showModel;
    S.certificate = Certificate(certifiedUNSATfile, certifiedUNSAT);
}

struct GateRecognitionArguments {
    const int opt_gr_tries;
    const bool opt_gr_patterns;
    const bool opt_gr_semantic;
    const bool opt_gr_holistic;
    const bool opt_gr_lookahead;
    const bool opt_gr_intensify;
    const int opt_gr_lookahead_threshold;
    const bool opt_print_gates;
};

static std::unique_ptr<Candy::GateAnalyzer> createGateAnalyzer(Candy::CNFProblem &dimacs, const GateRecognitionArguments& recognitionArgs) {
    return backported_std::make_unique<Candy::GateAnalyzer>(dimacs, recognitionArgs.opt_gr_tries, recognitionArgs.opt_gr_patterns,
                    recognitionArgs.opt_gr_semantic, recognitionArgs.opt_gr_holistic, recognitionArgs.opt_gr_lookahead, recognitionArgs.opt_gr_intensify,
                    recognitionArgs.opt_gr_lookahead_threshold);
}

/**
 * Performs gate recognition on the problem \p dimacs and prints statistics.
 *
 * TODO: document parameters
 */
static void benchmarkGateRecognition(Candy::CNFProblem &dimacs, const GateRecognitionArguments& recognitionArgs) {
    double recognition_time = Glucose::cpuTime();
    auto gates = createGateAnalyzer(dimacs, recognitionArgs);
    gates->analyze();
    recognition_time = Glucose::cpuTime() - recognition_time;
    printf("c ========================================[ Problem Statistics ]================================\n");
    printf("c |                                                                                            |\n");
    printf("c |  Number of gates:        %12d                                                      |\n", gates->getGateCount());
    printf("c |  Number of variables:    %12d                                                      |\n", dimacs.nVars());
    printf("c |  Number of clauses:      %12d                                                      |\n", dimacs.nClauses());
    printf("c |  Recognition time (sec): %12.2f                                                      |\n", recognition_time);
    printf("c |                                                                                            |\n");
    printf("c ==============================================================================================\n");
    if (recognitionArgs.opt_print_gates) {
        gates->printGates();
    }
}

struct RandomSimulationArguments {
    const int nRounds;
    const bool abortByRRAT;
    const double rrat;
    const bool filterConjecturesBySize;
    const int maxConjectureSize;
    const bool removeBackboneConjectures;
    const bool filterGatesByNonmono;
};

struct RSARArguments {
    const bool useRSAR;
    const int maxRefinementSteps;
    const Candy::SimplificationHandlingMode simplificationHandlingMode;
    const bool withInputDepCountHeuristic;
    const std::string inputDepCountHeuristicConfiguration;
};

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

static std::unique_ptr<Candy::Conjectures> performRandomSimulation(Candy::GateAnalyzer &analyzer, const RandomSimulationArguments& rsArguments) {
    auto simulatorBuilder = Candy::createDefaultRandomSimulatorBuilder();
    simulatorBuilder->withGateAnalyzer(analyzer);

    if (rsArguments.abortByRRAT) {
        simulatorBuilder->withReductionRateAbortThreshold(rsArguments.rrat);
    }

    if (rsArguments.filterGatesByNonmono) {
        simulatorBuilder->withGateFilter(Candy::createNonmonotonousGateFilter(analyzer));
    }

    auto randomSimulator = simulatorBuilder->build();
    auto conjectures = randomSimulator->run(static_cast<unsigned int>(rsArguments.nRounds));

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
    std::regex unsignedIntRegex { "^(\\s*[0-9]+\\s*)+$" };
    if (!std::regex_match(limitsString, unsignedIntRegex)) {
        throw std::invalid_argument(limitsString + ": invalid limits");
    }

    std::vector<size_t> limits = Candy::tokenizeByWhitespace<size_t>(limitsString);

    if (limits.size() == 0) {
        throw std::invalid_argument(limitsString + ": invalid limits");
    }

    return limits;
}

std::unique_ptr<Candy::ARSolver> createARSolver(Candy::GateAnalyzer& analyzer, SimpSolver& satSolver, std::unique_ptr<Candy::Conjectures> conjectures,
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

static lbool solveWithRSAR(SimpSolver& solver, Candy::CNFProblem& problem, const GateRecognitionArguments& gateRecognitionArgs,
                const RandomSimulationArguments& rsArguments, const RSARArguments& rsarArguments) {
    auto gateAnalyzer = createGateAnalyzer(problem, gateRecognitionArgs);
    gateAnalyzer->analyze();
    auto conjectures = performRandomSimulation(*gateAnalyzer, rsArguments);
    auto arSolver = createARSolver(*gateAnalyzer, solver, std::move(conjectures), rsarArguments);
    auto result = arSolver->solve();

    return lbool(result);
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

    const char *opt_certified_file;

    const GateRecognitionArguments gateRecognitionArgs;
    const RandomSimulationArguments randomSimulationArgs;
    const RSARArguments rsarArgs;
};

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

    BoolOption do_solve("METHOD", "solve", "Completely turn on/off actual sat solving.", true);
    BoolOption do_preprocess("METHOD", "pre", "Completely turn on/off any preprocessing.", true);
    BoolOption do_certified("METHOD", "certified", "Certified UNSAT using DRUP format", false);
    BoolOption do_gaterecognition("METHOD", "gates", "Completely turn on/off actual gate recognition.", false);

    StringOption opt_certified_file("CERTIFIED UNSAT", "certified-output", "Certified UNSAT output file", "NULL");

    BoolOption opt_print_gates("GATE RECOGNITION", "print-gates", "print gates.", false);
    IntOption opt_gr_tries("GATE RECOGNITION", "gate-tries", "Number of heuristic clause selections to enter recursion", 0, IntRange(0, INT32_MAX));
    BoolOption opt_gr_patterns("GATE RECOGNITION", "gate-patterns", "Enable Pattern-based Gate Detection", false);
    BoolOption opt_gr_semantic("GATE RECOGNITION", "gate-semantic", "Enable Semantic Gate Detection", false);
    BoolOption opt_gr_holistic("GATE RECOGNITION", "gate-holistic", "Enable Holistic Gate Detection", false);
    BoolOption opt_gr_lookahead("GATE RECOGNITION", "gate-lookahead", "Enable Local Blocked Elimination", false);
    IntOption opt_gr_lookahead_threshold("GATE RECOGNITION", "gate-lookahead-threshold", "Local Blocked Elimination Threshold", 10, IntRange(1, INT32_MAX));
    BoolOption opt_gr_intensify("GATE RECOGNITION", "gate-intensification", "Enable Local Blocked Elimination", false);

    IntOption opt_rs_nrounds("RANDOMSIMULATION", "rs-rounds", "Amount of random simulation rounds (gets rounded up to the next multiple of 2048)", 1048576,
                    IntRange(1, INT32_MAX));
    BoolOption opt_rs_abortbyrrat("RANDOMSIMULATION", "rs-abort-by-rrat", "Abort random simulation when the reduction rate falls below the RRAT threshold",
                    false);
    DoubleOption opt_rs_rrat("RANDOMSIMULATION", "rs-rrat", "Reduction rate abort threshold", 0.01, DoubleRange(0.0f, true, 1.0f, false));
    IntOption opt_rs_filterConjBySize("RANDOMSIMULATION", "rs-max-conj-size", "Max. allowed literal equivalence conjecture size (0: disable filtering by size)",
                    0, IntRange(0, INT32_MAX));
    BoolOption opt_rs_removeBackboneConj("RANDOMSIMULATION", "rs-remove-backbone-conj", "Filter out conjectures about the problem's backbone", false);
    BoolOption opt_rs_filterGatesByNonmono("RANDOMSIMULATION", "rs-only-nonmono-gates", "Use only nonmonotonously nested gates for random simulation", false);

    BoolOption opt_rsar_enable("RSAR", "rsar-enable", "Enable random-simulation-based abstraction refinement SAT solving", false);
    IntOption opt_rsar_maxRefinementSteps("RSAR", "rsar-max-refinements", "Max. refinement steps", 10, IntRange(1, INT32_MAX));
    StringOption opt_rsar_simpMode("RSAR", "rsar-simpmode", "Simplification handling mode", "RESTRICT");
    StringOption opt_rsar_inputDepCountHeurConf("RSAR", "rsar-heur-idc", "Input dependency count heuristic configuration", "");

    parseOptions(argc, argv, true);

    GateRecognitionArguments gateRecognitionArgs { opt_gr_tries, opt_gr_patterns, opt_gr_semantic, opt_gr_holistic, opt_gr_lookahead, opt_gr_intensify,
                    opt_gr_lookahead_threshold, opt_print_gates };

    RandomSimulationArguments rsArgs { opt_rs_nrounds, opt_rs_abortbyrrat, opt_rs_rrat, opt_rs_filterConjBySize > 0, opt_rs_filterConjBySize,
                    opt_rs_removeBackboneConj, opt_rs_filterGatesByNonmono };

    RSARArguments rsarArgs { opt_rsar_enable, opt_rsar_maxRefinementSteps, parseSimplificationHandlingMode(std::string { opt_rsar_simpMode }), std::string {
                    opt_rsar_inputDepCountHeurConf } != "", std::string { opt_rsar_inputDepCountHeurConf } };

    GlucoseArguments result { verb, mod, vv, cpu_lim, mem_lim, do_solve, do_preprocess, do_certified, do_gaterecognition, opt_certified_file,
                    gateRecognitionArgs, rsArgs, rsarArgs };

    return result;
}

//=================================================================================================
// Main:
int main(int argc, char** argv) {
    try {
        std::cout << "c Candy 0.2 is made of Glucose (Many thanks to the Glucose and MiniSAT teams)" << std::endl;

        Candy::Clause::printAlignment();

        GlucoseArguments args = parseCommandLineArgs(argc, argv);

        // Use signal handlers that forcibly quit until the solver will be able to respond to interrupts:
        installSignalHandlers(false);

        setLimits(args.cpu_lim, args.mem_lim);

        double initial_time = Glucose::cpuTime();

        SimpSolver S;
        solver = &S;
        configureSolver(S, args.verb,               // verbosity
                        args.vv,                 // verbosity every vv conflicts
                        args.mod,                // show model
                        0,                       // certifiedAllClauses
                        args.do_certified,       // certifiedUNSAT
                        args.opt_certified_file);       // certifiedUNSAT output file

        Candy::CNFProblem dimacs;
        if (argc == 1) {
            printf("c Reading from standard input... Use '--help' for help.\n");
            if (!dimacs.readDimacsFromStdout())
                return 1;
        } else {
            if (!dimacs.readDimacsFromFile(argv[1]))
                return 1;
        }

        if (args.do_gaterecognition) {
            benchmarkGateRecognition(dimacs, args.gateRecognitionArgs);
            return 0;
        }

        S.addClauses(dimacs);

        double parsed_time = Glucose::cpuTime();

        if (S.verbosity > 0) {
            printProblemStatistics(S, parsed_time - initial_time);
        }

        // Change to signal-handlers that will only notify the solver and allow it to terminate voluntarily
        installSignalHandlers(true);

        lbool result;
        if (!args.rsarArgs.useRSAR) {
            result = solve(S, args.do_preprocess, parsed_time);
        } else {
            result = solveWithRSAR(S, dimacs, args.gateRecognitionArgs, args.randomSimulationArgs, args.rsarArgs);
        }

        const char* statsFilename = (argc >= 3) ? argv[argc - 1] : nullptr;
        printResult(S, result, statsFilename);

        return (result == l_True ? 10 : result == l_False ? 20 : 0);
    } catch (std::bad_alloc& ba) {
        //printf("c Bad_Alloc Caught: %s\n", ba.what());
        printf("c ==============================================================================================\n");
        printf("s INDETERMINATE\n");
        return 0;
    }
}
