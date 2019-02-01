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
#include "candy/utils/System.h"
#include "candy/utils/Options.h"
#include "candy/utils/MemUtils.h"
#include "candy/core/branching/VSIDS.h"
#include "candy/core/branching/LRB.h"
#include "candy/core/Solver.h"
#include "candy/minimizer/Minimizer.h"

#include "candy/frontend/RandomSimulationFrontend.h"
#include "candy/frontend/SolverFactory.h"
#include "candy/frontend/CandyBuilder.h"
#include "candy/frontend/Exceptions.h"

#include "candy/gates/GateAnalyzer.h"
#include "candy/rsar/ARSolver.h"
#include "candy/rsar/Heuristics.h"


#include "candy/rsil/BranchingHeuristics.h"

#if !defined(WIN32)
#include <sys/resource.h>
#endif

using namespace Candy;

static std::function<void()> solverSetInterrupt;

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
    _exit(1);
}

/**
 * Installs signal handlers for SIGINT and SIGXCPU.
 * If \p handleInterruptsBySolver is true, the interrupts are handled by SIGINT_interrupt();
 * otherwise, they are set up to be handled by SIGINT_exit().
 */
static void installSignalHandlers(bool handleInterruptsBySolver, CandySolverInterface* solver) {
#if defined(WIN32) && !defined(CYGWIN)
#if defined(_MSC_VER)
#pragma message ("Warning: setting signal handlers not yet implemented for Win32")
#else
#warning "setting signal handlers not yet implemented for Win32"
#endif
#else
    if (handleInterruptsBySolver) {
        solverSetInterrupt = [solver]() {
            solver->setInterrupt(true);
        };

        signal(SIGINT, SIGINT_interrupt);
        signal(SIGXCPU, SIGINT_interrupt);
    } else {
        signal(SIGINT, SIGINT_exit);
        signal(SIGXCPU, SIGINT_exit);
    }
#endif
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

static void printProblemStatistics(CNFProblem& problem) {
    printf("c =====================[ Problem Statistics ]======================\n");
    printf("c |                                                               |\n");
    printf("c |  Number of variables:  %12zu                           |\n", problem.nVars());
    printf("c |  Number of clauses:    %12zu                           |\n", problem.nClauses());
}

static void runSolver(CandySolverInterface* solver, lbool& result, Cl& model) {
    // Change to signal-handlers that will only notify the solver and allow it to terminate voluntarily
    installSignalHandlers(true, solver);
    lbool inner_result = solver->solve();
    installSignalHandlers(false, solver);

    if (result == l_Undef) {
        result = inner_result;
        if (result == l_True) {
            model = solver->getModel();
        }

        if (SolverOptions::verb > 0) {
            Statistics::getInstance().printFinalStats(solver->nConflicts(), solver->nPropagations());
            Statistics::getInstance().printRuntimes();
        }
    }
}

int main(int argc, char** argv) {
    setUsageHelp("c USAGE: %s [options] <input-file>\n\nc where input may be either in plain or gzipped DIMACS.\n");
    parseOptions(argc, argv, true);
    
    if (SolverOptions::verb > 1) {
        std::cout << "c Candy 0.7 is made of Glucose (Many thanks to the Glucose and MiniSAT teams)" << std::endl;
    }

    setLimits(SolverOptions::cpu_lim, SolverOptions::mem_lim);

    Statistics::getInstance().runtimeStart("Initialization");

    CNFProblem problem{};
    try {
        if (argc == 1) {
            printf("c Reading from standard input... Use '--help' for help.\n");
            problem.readDimacsFromStdin();
        } else {
            const char* inputFilename = argv[1];
            problem.readDimacsFromFile(inputFilename);
        }
    }
    catch (ParserException& e) {
		printf("Caught Parser Exception\n%s\n", e.what());
        return 0;
    }

    if (SolverOptions::do_gaterecognition) {
        std::cout << "c benchmarking gate recognition. not impl. atm" << std::endl;
        return 0;
    }

    if (SolverOptions::verb > 0) {
        printProblemStatistics(problem);
    }    
    
    lbool result = l_Undef;
    Cl model;
    if (ParallelOptions::opt_threads == 1) {
        CandySolverInterface* solver = nullptr;

        if (RSAROptions::opt_rsar_enable) {
            solver = createRSARSolver(problem);
        }
        else {
            solver = createSolver(ParallelOptions::opt_static_propagate, SolverOptions::opt_use_lrb, RSILOptions::opt_rsil_enable, RSILOptions::opt_rsil_advice_size);
        }

        solver->init(problem);

        Statistics::getInstance().runtimeStop("Initialization");

        runSolver(solver, std::ref(result), std::ref(model));
    }
    else {
        GlobalClauseAllocator* global_allocator = nullptr;
        std::vector<CandySolverInterface*> solvers;
        std::vector<std::thread> threads;

        CandySolverInterface* solver = createSolver(ParallelOptions::opt_static_propagate, false, false); 
        solver->init(problem);
        if (ParallelOptions::opt_static_database) {
            global_allocator = solver->setupGlobalAllocator();
        }
        solvers.push_back(solver);
        threads.push_back(std::thread(runSolver, solver, std::ref(result), std::ref(model)));

        for (int count = 2; count <= ParallelOptions::opt_threads; count++) {
            switch (count) {
                case 2 : // plain subsumption inprocessing
                    SolverOptions::opt_sort_watches = false;
                    SolverOptions::opt_sort_variables = false;
                    SolverOptions::opt_inprocessing = 1;
                    SolverOptions::opt_use_lrb = false;
                    SolverOptions::opt_preprocessing = false;
                    RSILOptions::opt_rsil_enable = false;
                    VariableEliminationOptions::opt_use_elim = false;
                    VariableEliminationOptions::opt_use_asymm = false;
                    break;
                case 3 : // plain lrb
                    SolverOptions::opt_sort_watches = true;
                    SolverOptions::opt_sort_variables = true;
                    SolverOptions::opt_inprocessing = 0;
                    SolverOptions::opt_use_lrb = true;
                    SolverOptions::opt_preprocessing = false;
                    RSILOptions::opt_rsil_enable = false;
                    VariableEliminationOptions::opt_use_elim = false;
                    VariableEliminationOptions::opt_use_asymm = false;
                    break;
                case 4 : // plain rsil
                    SolverOptions::opt_sort_watches = false;
                    SolverOptions::opt_sort_variables = false;
                    SolverOptions::opt_inprocessing = 0;
                    SolverOptions::opt_use_lrb = false;
                    SolverOptions::opt_preprocessing = false;
                    RSILOptions::opt_rsil_enable = true;
                    VariableEliminationOptions::opt_use_elim = false;
                    VariableEliminationOptions::opt_use_asymm = false;
                    break;
                case 5 : // full inprocessing
                    SolverOptions::opt_sort_watches = true;
                    SolverOptions::opt_sort_variables = true;
                    SolverOptions::opt_sort_watches = false;
                    SolverOptions::opt_sort_variables = false;
                    SolverOptions::opt_inprocessing = 300;
                    SolverOptions::opt_use_lrb = false;
                    SolverOptions::opt_preprocessing = false;
                    RSILOptions::opt_rsil_enable = false;
                    VariableEliminationOptions::opt_use_elim = true;
                    VariableEliminationOptions::opt_use_asymm = true;
                    break;
                case 6 : // full inprocessing with lrb
                    SolverOptions::opt_sort_watches = true;
                    SolverOptions::opt_sort_variables = true;
                    SolverOptions::opt_inprocessing = 300;
                    SolverOptions::opt_use_lrb = true;
                    SolverOptions::opt_preprocessing = false;
                    RSILOptions::opt_rsil_enable = false;
                    VariableEliminationOptions::opt_use_elim = true;
                    VariableEliminationOptions::opt_use_asymm = true;
                    break;
                case 7 : // full inprocessing with rsil
                    SolverOptions::opt_sort_watches = false;
                    SolverOptions::opt_sort_variables = false;
                    SolverOptions::opt_inprocessing = 300;
                    SolverOptions::opt_use_lrb = false;
                    SolverOptions::opt_preprocessing = false;
                    RSILOptions::opt_rsil_enable = true;
                    VariableEliminationOptions::opt_use_elim = true;
                    VariableEliminationOptions::opt_use_asymm = true;
                    break;
                case 8 : // plain w/o sorting
                    SolverOptions::opt_sort_watches = false;
                    SolverOptions::opt_sort_variables = false;
                    SolverOptions::opt_inprocessing = 0;
                    SolverOptions::opt_use_lrb = false;
                    SolverOptions::opt_preprocessing = false;
                    RSILOptions::opt_rsil_enable = false;
                    VariableEliminationOptions::opt_use_elim = false;
                    VariableEliminationOptions::opt_use_asymm = false;
                    break;
            }
            solver = createSolver(ParallelOptions::opt_static_propagate, SolverOptions::opt_use_lrb, RSILOptions::opt_rsil_enable);
            solver->init(problem, global_allocator);
            solvers.push_back(solver);
            threads.push_back(std::thread(runSolver, solver, std::ref(result), std::ref(model)));
        }

        Statistics::getInstance().runtimeStop("Initialization");

        while (result == l_Undef) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        for (CandySolverInterface* solver : solvers) {
            solver->setInterrupt(true);
        }
        
        for (std::thread& thread : threads) {
            thread.detach();
        }
    }

    printf(result == l_True ? "s SATISFIABLE\n" : result == l_False ? "s UNSATISFIABLE\n" : "s INDETERMINATE\n");

    if (result == l_True && SolverOptions::mod) {
        if (SolverOptions::do_minimize > 0) {
            Minimizer minimizer(problem, model);
            Cl minimalModel = minimizer.computeMinimalModel(SolverOptions::do_minimize == 2);
            for (Lit lit : minimalModel) {
                printLiteral(lit);
            }
        } 
        else {
            std::cout << "v ";
            for (Lit lit : model) {
                printLiteral(lit);
            }
            std::cout << " 0" << std::endl;
        }
    }

    #ifndef __SANITIZE_ADDRESS__
        exit((result == l_True ? 10 : result == l_False ? 20 : 0));
    #endif
    return (result == l_True ? 10 : result == l_False ? 20 : 0);
}

