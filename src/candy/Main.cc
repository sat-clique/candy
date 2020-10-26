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
#include <csignal>
#include <string>
#include <stdexcept>
#include <regex>
#include <functional>
#include <type_traits>
#include <chrono>

#include "candy/utils/Memory.h"
#include "candy/utils/Options.h"

#include "candy/utils/StreamBuffer.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/core/CandySolverResult.h"
#include "candy/core/DRATChecker.h"

#include "candy/utils/CandyBuilder.h"
#include "candy/utils/Runtime.h"

using namespace Candy;

static std::vector<CandySolverInterface*> solvers;

static bool interrupted = false;
static unsigned int start_time = 0;
static int interrupted_callback(void* state) {
    if (SolverOptions::time_limit > 0) {
        if (start_time == 0) {
            start_time = get_wall_time();
        }
        else if (get_wall_time() - start_time > SolverOptions::time_limit) {
            interrupted = true;
        }
    }
    if (SolverOptions::memory_limit > 0 && static_cast<int>(getCurrentRSS()/(1024*1024)) > SolverOptions::memory_limit) {
        interrupted = true;
    }
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
#pragma message ("Warning: setting signal handlers not implemented for Win32")
#else
#warning "setting signal handlers not implemented for Win32"
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
    std::cout << "c Variables: " << problem.nVars() << std::endl;
    std::cout << "c Clauses: " << problem.nClauses() << std::endl;
}
    
CandySolverInterface* solver = nullptr;
lbool result = l_Undef;

static void runSolverThread(lbool& result, CandySolverInterface*& solver, CNFProblem& problem, ClauseAllocator*& global_allocator) {
    std::cout << "c Sort Variables: " << SolverOptions::opt_sort_variables << std::endl;
    std::cout << "c Preprocessing: " << SolverOptions::opt_preprocessing << std::endl;
    std::cout << "c Inprocessing: " << SolverOptions::opt_inprocessing << std::endl;

    CandySolverInterface* solver_ = createSolver(ParallelOptions::opt_static_propagate, SolverOptions::opt_use_lrb);

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
    std::cout << "c Candy is made from Glucose." << std::endl;

    setUsageHelp("c USAGE: %s [options] <input-file>\n\nc where input may be either in plain or compressed DIMACS.\n");
    parseOptions(argc, argv, true);

    CNFProblem problem{};
    try {
        std::cout << "c Reading file: " << argv[1] << std::endl; 
        problem.readDimacsFromFile(argv[1]);
    }
    catch (ParserException& e) {
		std::cout << "c Caught Parser Exception: " << std::endl << e.what() << std::endl;
        return 1;
    }

    if (TestingOptions::test_proof && strlen(SolverOptions::opt_certified_file) == 0) {
        SolverOptions::opt_certified_file = "proof.drat";
    }

    if (TestingOptions::test_limit > 0 && (int)problem.nVars() > TestingOptions::test_limit) {
        std::cout << "c Number of variables surpasses testing limit" << std::endl;
        return 0;
    }

    if (SolverOptions::verb == 1) {
        printProblemStatistics(problem);
    }

    ClauseAllocator* global_allocator = nullptr;
    std::vector<std::thread> threads;

    if (ParallelOptions::opt_threads == 1) {
        solver = createSolver(ParallelOptions::opt_static_propagate, SolverOptions::opt_use_lrb);
        solvers.push_back(solver); 

        solver->init(problem);
        solver->dumpBinWatchStats("wb ");
        solver->setTermCallback(solver, interrupted_callback);

        installSignalHandlers(true);
        result = solver->solve();
        installSignalHandlers(false);
        solver->dumpBinWatchStats("wa ");
    }
    else {
        if (ParallelOptions::opt_static_database) {
            std::cout << "Parallel mode with static database is disabled in this version due to a bug. If you want to test it, please checkout the branch 'release_sat_2019'." << std::endl;
            exit(0);
        }
        for (unsigned int count = 0; count < (unsigned int)ParallelOptions::opt_threads && result == l_Undef; count++) {
            std::cout << "c Initializing Solver " << count << std::endl;
            SolverOptions::opt_sort_variables = ((count % 2) == 0);
            SolverOptions::opt_preprocessing = (count == 0);
            SolverOptions::opt_inprocessing = count + SolverOptions::opt_inprocessing;
            VariableEliminationOptions::opt_use_elim = !ParallelOptions::opt_static_database;
            switch (count) {
                case 0 : case 1 : 
                    SolverOptions::opt_use_lrb = false;
                    break;
                case 2 : case 3 : 
                    SolverOptions::opt_use_lrb = true;
                    break;
                case 4 : case 5 : 
                    SolverOptions::opt_vsids_var_decay = 0.75;
                    SolverOptions::opt_use_lrb = false;
                    break;
                case 6 : case 7 : 
                    SolverOptions::opt_vsids_var_decay = 0.9;
                    SolverOptions::opt_use_lrb = false;
                    break;
            }
            
            threads.push_back(std::thread(runSolverThread, std::ref(result), std::ref(solver), std::ref(problem), std::ref(global_allocator)));

            do { 
                // wait till solver is initialized (and wait a bit longer)
                std::this_thread::sleep_for(std::chrono::milliseconds(ParallelOptions::opt_thread_initialization_delay)); 
            } while (solvers.size() <= count);
        }

        installSignalHandlers(true);
        // problem.clear();
        while (result == l_Undef && !interrupted) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        installSignalHandlers(false);
    }

    printf(result == l_True ? "s SATISFIABLE\n" : result == l_False ? "s UNSATISFIABLE\n" : "s INDETERMINATE\n");

    if (solver != nullptr) {
        CandySolverResult& model = solver->getCandySolverResult();

        if (result == l_True && SolverOptions::mod) {
            std::cout << "v";
            for (Var v = 0; v < (Var)problem.nVars(); v++) {
                std::cout << " " << model.value(v);
            }
            std::cout << " 0" << std::endl;
        }

        if (SolverOptions::verb > 0) {
            solver->printStats();
        }

        if (TestingOptions::test_model && result == l_True) {
            bool satisfied = problem.checkResult(model);
            if (satisfied) {
                std::cout << "c Result verified by model checker" << std::endl;
                std::cout << "c ********************************" << std::endl;
                return 10;
            }
            else {
                std::cout << "c Result could not be verified by model checker" << std::endl;
                if (TestingOptions::test_limit > 0) assert(satisfied); //fuzzing
                return 11;
            }
        }
        else if (TestingOptions::test_proof && result == l_False) {
            std::string file (SolverOptions::opt_certified_file);
            SolverOptions::opt_certified_file = "";
            DRATChecker checker(problem);
            bool proved = checker.check_proof(file.c_str());
            // bool proved = (0 == check_proof((char*)inputFilename, SolverOptions::opt_certified_file.get())); 
            if (proved) {
                std::cout << "c Result verified by proof checker" << std::endl;
                std::cout << "c ********************************" << std::endl;
                return 20;
            }
            else {
                std::cout << "c Result could not be verified by proof checker" << std::endl;
                if (TestingOptions::test_limit > 0) assert(proved); //fuzzing
                return 21;
            }
        }
    }

    #ifndef __SANITIZE_ADDRESS__
        exit((result == l_True ? 10 : result == l_False ? 20 : 0));
    #endif
    return (result == l_True ? 10 : result == l_False ? 20 : 0);
}

