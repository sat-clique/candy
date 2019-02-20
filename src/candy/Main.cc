/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Candy sources are based on Glucose which is based on MiniSat (see former copyrights below). 
Permissions and copyrights of Candy are exactly the same as Glucose and Minisat (see below).


--------------- Former Glucose Copyrights

 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
                                CRIL - Univ. Artois, France
                                LRI  - Univ. Paris Sud, France (2009-2013)
                                Labri - Univ. Bordeaux, France

Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it 
is based on. (see below).


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
*************************************************************************************************/

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
#include "candy/core/Statistics.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/core/CandySolverResult.h"
#include "candy/utils/Options.h"
#include "candy/utils/MemUtils.h"
#include "candy/minimizer/Minimizer.h"

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

static std::vector<CandySolverInterface*> solvers;

static bool interrupted = false;
static int interrupted_callback(void* state) {
    return interrupted ? 1 : 0;
}

static void set_interrupted(int signum) {
    std::cout << "c Informing solver about desire to interrupt." << std::endl;
    interrupted = true;
}

static void just_quit(int signum) {
    std::cout << std::endl << "*** INTERRUPTED ***" << std::endl;
    _exit(1);
}

static void installSignalHandlers(bool handleInterruptsBySolver) {
#if defined(WIN32) && !defined(CYGWIN)
#if defined(_MSC_VER)
#pragma message ("Warning: setting signal handlers not yet implemented for Win32")
#else
#warning "setting signal handlers not yet implemented for Win32"
#endif
#else
    if (handleInterruptsBySolver) {
        signal(SIGINT, set_interrupted);
        signal(SIGXCPU, set_interrupted);
    } else {
        signal(SIGINT, just_quit);
        signal(SIGXCPU, just_quit);
    }
#endif
}

static void printProblemStatistics(CNFProblem& problem) {
    printf("c =====================[ Problem Statistics ]======================\n");
    printf("c |                                                               |\n");
    printf("c |  Number of variables:  %12zu                           |\n", problem.nVars());
    printf("c |  Number of clauses:    %12zu                           |\n", problem.nClauses());
}

static void runSolverThread(lbool& result, CandySolverInterface*& solver, CNFProblem& problem, ClauseAllocator*& global_allocator) {
    std::cout << "c Sort Watches: " << SolverOptions::opt_sort_watches << std::endl;
    std::cout << "c Sort Variables: " << SolverOptions::opt_sort_variables << std::endl;
    std::cout << "c Preprocessing: " << SolverOptions::opt_preprocessing << std::endl;
    std::cout << "c Inprocessing: " << SolverOptions::opt_inprocessing << std::endl;

    CandySolverInterface* solver_ = createSolver(ParallelOptions::opt_static_propagate, SolverOptions::opt_use_lrb, RSILOptions::opt_rsil_enable);

    solver_->init(problem, global_allocator);

    if (ParallelOptions::opt_static_database && global_allocator == nullptr) {
        global_allocator = solver_->setupGlobalAllocator();
    }

    solvers.push_back(solver_);
    solver_->setTermCallback(solver_, interrupted_callback);

    lbool result_ = solver_->solve();

    if (result == l_Undef) {
        result = result_;
        solver = solver_;
    }
}

int main(int argc, char** argv) {
    setUsageHelp("c USAGE: %s [options] <input-file>\n\nc where input may be either in plain or gzipped DIMACS.\n");
    parseOptions(argc, argv, true);
    
    if (SolverOptions::verb > 1) {
        std::cout << "c Candy 0.7 is made of Glucose (Many thanks to the Glucose and MiniSAT teams)" << std::endl;
    }

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

    if (SolverOptions::verb > 0) {
        printProblemStatistics(problem);
    }

    ClauseAllocator* global_allocator = nullptr;
    std::vector<std::thread> threads;
    
    CandySolverInterface* solver = nullptr;
    lbool result = l_Undef;

    if (ParallelOptions::opt_threads == 1) {
        if (RSAROptions::opt_rsar_enable) {
            solver = createRSARSolver(problem);
        }
        else {
            solver = createSolver(ParallelOptions::opt_static_propagate, SolverOptions::opt_use_lrb, RSILOptions::opt_rsil_enable, RSILOptions::opt_rsil_advice_size);
        }
        solvers.push_back(solver); 

        solver->init(problem);
        solver->setTermCallback(solver, interrupted_callback);

        installSignalHandlers(true);
        result = solver->solve();
        installSignalHandlers(false);
    }
    else {
        for (unsigned int count = 0; count < (unsigned int)ParallelOptions::opt_threads; count++) {
            std::cout << "c Initializing Solver " << count << std::endl;
            ClauseDatabaseOptions::opt_recalculate_lbd = false; 
            SolverOptions::opt_unitresolution = 0;
            SolverOptions::opt_sort_watches = ((count % 2) == 0);
            SolverOptions::opt_sort_variables = ((count % 2) == 0);
            SolverOptions::opt_preprocessing = ((count % 2) == 0);
            SolverOptions::opt_inprocessing = (count % 2) * 300;
            VariableEliminationOptions::opt_use_elim = !ParallelOptions::opt_static_database;
            VariableEliminationOptions::opt_use_asymm = (count % 3 == 0);
            switch (count) {
                case 0 : case 1 : //vsids
                    SolverOptions::opt_use_lrb = false;
                    RSILOptions::opt_rsil_enable = false;
                    break;
                case 2 : case 3 : //lrb
                    SolverOptions::opt_use_lrb = true;
                    RSILOptions::opt_rsil_enable = false;
                    break;
                case 4 : case 5 : //rsil
                    SolverOptions::opt_use_lrb = false;
                    RSILOptions::opt_rsil_enable = true;
                    break;
                case 6 : case 7 : //vsids
                    SolverOptions::opt_use_lrb = false;
                    RSILOptions::opt_rsil_enable = false;
                    break;
            }
            
            threads.push_back(std::thread(runSolverThread, std::ref(result), std::ref(solver), std::ref(problem), std::ref(global_allocator)));

            while (solvers.size() <= count) { 
                // wait till solver is initialized
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        for (std::thread& thread : threads) {
            thread.detach();
        }

        installSignalHandlers(true);
        while (result == l_Undef && !interrupted) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        installSignalHandlers(false);
    }

    printf(result == l_True ? "s SATISFIABLE\n" : result == l_False ? "s UNSATISFIABLE\n" : "s INDETERMINATE\n");

    if (solver != nullptr) {
        if (result == l_True && SolverOptions::mod) {
            CandySolverResult& result = solver->getCandySolverResult();
            if (SolverOptions::do_minimize > 0) {
                Minimizer minimizer(problem, result.getModelLiterals());
                Cl minimalModel = minimizer.computeMinimalModel(SolverOptions::do_minimize == 2);
                for (Lit lit : minimalModel) {
                    printLiteral(lit);
                }
            } 
            else {
                Cl model = result.getModelLiterals();
                std::cout << "v ";
                for (Lit lit : model) {
                    printLiteral(lit);
                }
                std::cout << " 0" << std::endl;
            }
        }

        if (SolverOptions::verb > 0) {
            solver->getStatistics().printFinalStats();
        }

        solver->getStatistics().printRuntimes();
    }

    #ifndef __SANITIZE_ADDRESS__
        if (ParallelOptions::opt_threads > 1) std::terminate();
        exit((result == l_True ? 10 : result == l_False ? 20 : 0));
    #endif
    return (result == l_True ? 10 : result == l_False ? 20 : 0);
}

