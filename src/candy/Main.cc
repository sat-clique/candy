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

#include "candy/core/CNFProblem.h"
#include "candy/core/Certificate.h"
#include "candy/core/Statistics.h"
#include "candy/core/ClauseAllocator.h"
#include "candy/utils/System.h"
#include "candy/utils/ParseUtils.h"
#include "candy/utils/Options.h"
#include "candy/utils/MemUtils.h"
#include "candy/simp/SimpSolver.h"

#include "candy/frontend/GateAnalyzerFrontend.h"
#include "candy/frontend/RSILFrontend.h"
#include "candy/frontend/RandomSimulationFrontend.h"
#include "candy/frontend/RSARFrontend.h"
#include "candy/frontend/CandyCommandLineParser.h"

#include "candy/gates/GateAnalyzer.h"
#include "candy/rsar/ARSolver.h"
#include "candy/rsar/SolverAdapter.h"
#include "candy/rsar/Heuristics.h"


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
        Statistics::getInstance().runtimeStart("Preprocessing");
        S.eliminate();
        S.disablePreprocessing();
        Statistics::getInstance().runtimeStop("Preprocessing");
        if (S.isInConflictingState()) {
            result = l_False;
            S.certificate->proof();
        }
        if (S.verbosity > 0) {
            Statistics::getInstance().printRuntime("Preprocessing");
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
        Statistics::getInstance().printRuntimes();
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

template<class SolverType> static
typename std::enable_if<std::is_same<SolverType, DefaultSimpSolver>::value, lbool>::type
solveWithRSAR(SolverType& solver, std::unique_ptr<Candy::CNFProblem> problem, const GateRecognitionArguments& gateRecognitionArgs,
                const RandomSimulationArguments& rsArguments, const RSARArguments& rsarArguments) {
    // TODO: the CPU time code was inserted in quite a hurry and
    // needs to be refactored.
    
    std::chrono::milliseconds startCPUTime = Glucose::cpuTime();
    
    GateRecognitionArguments localGateRecognitionArgs = gateRecognitionArgs;
    if (rsArguments.preprocessingTimeLimit >= std::chrono::milliseconds{0}) {
        localGateRecognitionArgs.opt_gr_timeout = std::min(gateRecognitionArgs.opt_gr_timeout,
                                                           rsArguments.preprocessingTimeLimit);
    }

    auto gateAnalyzer = createGateAnalyzer(*problem, localGateRecognitionArgs);
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
solveWithRSAR(SolverType& solver, std::unique_ptr<Candy::CNFProblem> problem, const GateRecognitionArguments& gateRecognitionArgs,
              const RandomSimulationArguments& rsArguments, const RSARArguments& rsarArguments) {
    throw std::logic_error("solveWithRSAR may only be called with SolverType == DefaultSimpSolver");
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
    S.strengthen();
    S.printDIMACS();

    return result;
}


template<class SolverType = DefaultSimpSolver>
int executeSolver(const GlucoseArguments& args,
                  SolverType& S,
                  std::unique_ptr<CNFProblem> problem) {
    Statistics::getInstance().runtimeStart("Initialization");
    S.addClauses(*problem);
    Statistics::getInstance().runtimeStop("Initialization");
    
    if (S.verbosity > 0) {
        printProblemStatistics(S);
        Statistics::getInstance().printRuntime("Initialization");
        printf("c |                                                                                            |\n");
    }
    
    // Change to signal-handlers that will only notify the solver and allow it to terminate voluntarily
    installSignalHandlers(true, &S);
    
    lbool result;
    if (args.do_simp_out) {
        result = simplifyAndPrintProblem(S);
    } else if (!args.rsarArgs.useRSAR) {
        problem.reset(nullptr); // free clauses
        result = solve(S, args.do_preprocess);
    } else {
        result = solveWithRSAR(S, std::move(problem), args.gateRecognitionArgs, args.randomSimulationArgs, args.rsarArgs);
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

        Statistics::getInstance().runtimeStart("Initialization");

        std::unique_ptr<Certificate> certificate = backported_std::make_unique<Certificate>(args.opt_certified_file,
                                                                                            args.do_certified);

        std::unique_ptr<Candy::CNFProblem> problem = backported_std::make_unique<Candy::CNFProblem>(*certificate);
        if (args.read_from_stdin) {
            printf("c Reading from standard input... Use '--help' for help.\n");
            if (!problem->readDimacsFromStdout()) {
                return 1;
            }
        } else {
            if (!problem->readDimacsFromFile(args.input_filename)) {
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
                preprocessingHook(*S, *problem);
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

        Statistics::getInstance().runtimeStop("Initialization");

        if (args.do_gaterecognition) {
            benchmarkGateRecognition(*problem, args.gateRecognitionArgs);
            return 0;
        }
        
        if (!fallBackToUnmodifiedCandy) {
            return executeSolver(args, *S, std::move(problem));
        }
        else {
            return executeSolver(args, *fallbackSolver, std::move(problem));
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
        auto preprocessor = createRSILPreprocessingHook<RSILBudgetSolver>(args.gateRecognitionArgs,
                                                                         args.randomSimulationArgs,
                                                                         args.rsilArgs);
        return solve<RSILBudgetSolver>(args, preprocessor);
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
        throw std::invalid_argument("Using RSAR with RSIL is not yet supported");
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

