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

#include <signal.h>
#include <zlib.h>
#include <sys/resource.h>

#include "core/CNFProblem.h"
#include "utils/System.h"
#include "utils/ParseUtils.h"
#include "utils/Options.h"
#include "simp/SimpSolver.h"

#include "gates/GateAnalyzer.h"

using namespace Glucose;

//=================================================================================================

void printStats(Solver& solver) {
  double cpu_time = cpuTime();
  double mem_used = 0; //memUsedPeak();
  printf("c restarts              : %" PRIu64" (%" PRIu64" conflicts in avg)\n", solver.starts, (solver.starts > 0 ? solver.conflicts / solver.starts : 0));
  printf("c blocked restarts      : %" PRIu64" (multiple: %" PRIu64") \n", solver.nbstopsrestarts, solver.nbstopsrestartssame);
  printf("c last block at restart : %" PRIu64"\n", solver.lastblockatrestart);
  printf("c nb ReduceDB           : %" PRIu64"\n", solver.nbReduceDB);
  printf("c nb removed Clauses    : %" PRIu64"\n", solver.nbRemovedClauses);
  printf("c nb learnts DL2        : %" PRIu64"\n", solver.nbDL2);
  printf("c nb learnts size 2     : %" PRIu64"\n", solver.nbBin);
  printf("c nb learnts size 1     : %" PRIu64"\n", solver.nbUn);

  printf("c conflicts             : %-12" PRIu64"   (%.0f /sec)\n", solver.conflicts, solver.conflicts / cpu_time);
  printf("c decisions             : %-12" PRIu64"   (%4.2f %% random) (%.0f /sec)\n", solver.decisions,
      (float) solver.rnd_decisions * 100 / (float) solver.decisions, solver.decisions / cpu_time);
  printf("c propagations          : %-12" PRIu64"   (%.0f /sec)\n", solver.propagations, solver.propagations / cpu_time);
  printf("c conflict literals     : %-12" PRIu64"   (%4.2f %% deleted)\n", solver.tot_literals,
      (solver.max_literals - solver.tot_literals) * 100 / (double) solver.max_literals);
  printf("c nb reduced Clauses    : %" PRIu64"\n", solver.nbReducedClauses);

  if (mem_used != 0)
    printf("Memory used           : %.2f MB\n", mem_used);
  printf("c CPU time              : %g s\n", cpu_time);
}

static Solver* solver;

// Terminate by notifying the solver and back out gracefully. This is mainly to have a test-case
// for this feature of the Solver as it may take longer than an immediate call to '_exit()'.
static void SIGINT_interrupt(int signum) {
  solver->interrupt();
}

// Note that '_exit()' rather than 'exit()' has to be used. The reason is that 'exit()' calls
// destructors and may cause deadlocks if a malloc/free function happens to be running (these
// functions are guarded by locks for multithreaded use).
static void SIGINT_exit(int signum) {
  printf("\n");
  printf("*** INTERRUPTED ***\n");
  if (solver->verbosity > 0) {
    printStats(*solver);
    printf("\n");
    printf("*** INTERRUPTED ***\n");
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
    if (!S.okay()) result = l_False;
    double simplified_time = cpuTime();
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
static void printResult(SimpSolver& S, lbool result, const char* outputFilename = nullptr) {
  if (S.verbosity > 0) {
    printStats(S);
    printf("\n");
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
  }
  else {
    signal(SIGINT, SIGINT_exit);
    signal(SIGXCPU, SIGINT_exit);
  }
}

/**
 * Prints statistics about the problem to be solved.
 */
static void printProblemStatistics(SimpSolver& S, double parseTime) {
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
static void configureSolver(SimpSolver& S,
                            int verbosity,
                            int verbosityEveryConflicts,
                            bool showModel,
                            bool certifiedAllClauses,
                            bool certifiedUNSAT,
                            const char* certifiedUNSATfile) {
  S.verbosity = verbosity;
  S.verbEveryConflicts = verbosityEveryConflicts;
  S.showModel = showModel;
  S.certifiedAllClauses = certifiedAllClauses;
  S.certifiedUNSAT = certifiedUNSAT;
  if (S.certifiedUNSAT) {
    if (!strcmp(certifiedUNSATfile, "NULL")) {
      S.certifiedOutput = fopen("/dev/stdout", "wb");
    } else {
      S.certifiedOutput = fopen(certifiedUNSATfile, "wb");
    }
    fprintf(S.certifiedOutput, "o proof DRUP\n");
  }
}

/**
 * Shuts down the SAT solver \p S and frees resources as necessary.
 */
static void shutdownSolver(SimpSolver& S) {
  // TODO: shouldn't this be done by the solver - maybe add a "shutdown()" method?
  if (S.certifiedUNSAT) {
    fprintf(S.certifiedOutput, "0\n");
    fclose(S.certifiedOutput);
  }
}

struct GateRecognitionArguments {
  int opt_gr_tries;
  bool opt_gr_patterns;
  bool opt_gr_semantic;
  bool opt_gr_holistic;
  bool opt_gr_lookahead;
  bool opt_gr_intensify;
  int opt_gr_lookahead_threshold;
  bool opt_print_gates;
};

/**
 * Performs gate recognition on the problem \p dimacs and prints statistics.
 *
 * TODO: document parameters
 */
static void benchmarkGateRecognition(Candy::CNFProblem &dimacs,
                                     const GateRecognitionArguments& recognitionArgs) {
  double recognition_time = cpuTime();
  Candy::GateAnalyzer gates(dimacs,
                            recognitionArgs.opt_gr_tries,
                            recognitionArgs.opt_gr_patterns,
                            recognitionArgs.opt_gr_semantic,
                            recognitionArgs.opt_gr_holistic,
                            recognitionArgs.opt_gr_lookahead,
                            recognitionArgs.opt_gr_intensify,
                            recognitionArgs.opt_gr_lookahead_threshold);
  gates.analyze();
  recognition_time = cpuTime() - recognition_time;
  printf("c ========================================[ Problem Statistics ]===========================================\n");
  printf("c |                                                                                                       |\n");
  printf("c |  Number of gates:        %12d                                                                 |\n", gates.getGateCount());
  printf("c |  Number of variables:    %12d                                                                 |\n", dimacs.nVars());
  printf("c |  Number of clauses:      %12d                                                                 |\n", dimacs.nClauses());
  printf("c |  Recognition time (sec): %12.2f                                                                 |\n", recognition_time);
  printf("c |                                                                                                       |\n");
  printf("c =========================================================================================================\n");
  if (recognitionArgs.opt_print_gates) {
    gates.printGates();
  }
}

//=================================================================================================
// Main:

int main(int argc, char** argv) {
  try {
    printf("c\nc This is Candy 0.1 -- based on Glucose (Many thanks to the Glucose and MiniSAT teams)\nc\n");

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

    parseOptions(argc, argv, true);

    // Use signal handlers that forcibly quit until the solver will be able to respond to interrupts:
    installSignalHandlers(false);

    setLimits(cpu_lim, mem_lim);

    double initial_time = cpuTime();

    SimpSolver S;
    solver = &S;
    configureSolver(S,
                    verb,               // verbosity
                    vv,                 // verbosity every vv conflicts
                    mod,                // show model
                    0,                  // certifiedAllClauses
                    do_certified,       // certifiedUNSAT
                    opt_certified_file);// certifiedUNSAT output file


    Candy::CNFProblem dimacs;
    if (argc == 1) {
      printf("c Reading from standard input... Use '--help' for help.\n");
      if (!dimacs.readDimacsFromStdout()) return 1;
    }
    else {
      if (!dimacs.readDimacsFromFile(argv[1])) return 1;
    }

    GateRecognitionArguments gateRecognitionArgs{opt_gr_tries, opt_gr_patterns, opt_gr_semantic,
        opt_gr_holistic, opt_gr_lookahead, opt_gr_intensify, opt_gr_lookahead_threshold,
        opt_print_gates};
    if (do_gaterecognition) {
      benchmarkGateRecognition(dimacs, gateRecognitionArgs);
      return 0;
    }

    S.insertClauses(dimacs);

    double parsed_time = cpuTime();

    if (S.verbosity > 0) {
      printProblemStatistics(S, parsed_time - initial_time);
    }

    // Change to signal-handlers that will only notify the solver and allow it to terminate voluntarily
    installSignalHandlers(true);

    lbool result = solve(S, do_preprocess, parsed_time);

    const char* statsFilename = (argc >= 3) ? argv[argc - 1] : nullptr;
    printResult(S, result, statsFilename);
    shutdownSolver(S);

    return (result == l_True ? 10 : result == l_False ? 20 : 0);
  }
  catch (OutOfMemoryException&) {
    printf("c =========================================================================================================\n");
    printf("s INDETERMINATE\n");
    return 0;
  }
}
