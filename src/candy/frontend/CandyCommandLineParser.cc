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



#include "CandyCommandLineParser.h"

namespace Candy {
    std::ostream& operator <<(std::ostream& stream, const GlucoseArguments& arguments) {
        stream << "c Glucose arguments: " << std::endl
        << "c   Verbosity: " << arguments.verb << std::endl
        << "c   Show model: " << arguments.mod << std::endl
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

    GlucoseArguments parseCommandLineArgs(int& argc, char** argv) {
        using namespace Glucose;
        setUsageHelp("c USAGE: %s [options] <input-file> <result-output-file>\n\n  where input may be either in plain or gzipped DIMACS.\n");
        
        // Extra options:
        //
        IntOption verb("MAIN", "verb", "Verbosity level (0=silent, 1=some, 2=more).", 1, IntRange(0, 2));
        BoolOption mod("MAIN", "model", "show model.", false);
        
        IntOption cpu_lim("MAIN", "cpu-lim", "Limit on CPU time allowed in seconds.\n", INT32_MAX, IntRange(0, INT32_MAX));
        IntOption mem_lim("MAIN", "mem-lim", "Limit on memory usage in megabytes.\n", INT32_MAX, IntRange(0, INT32_MAX));
        BoolOption wait_for_user("MAIN", "wait", "Wait for user input on startup (for profiling).", false);
        
        BoolOption do_solve("METHOD", "solve", "Completely turn on/off actual sat solving.", true);
        BoolOption do_preprocess("METHOD", "pre", "Completely turn on/off any preprocessing.", true);
        BoolOption do_certified("METHOD", "certified", "Certified UNSAT using DRUP format", false);
        BoolOption do_gaterecognition("METHOD", "gates", "Completely turn on/off actual gate recognition.", false);
        BoolOption do_simp_out("METHOD", "simp-out", "Simplify only and output dimacs.", false);
        IntOption do_minimize("MAIN", "minimize", "Model Minimization (0=none, 1=normal, 2=pruning).", 0, IntRange(0, 2));
        
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

//        Branch::defaultParameters = Branch::Parameters(SolverOptions::opt_var_decay, SolverOptions::opt_max_var_decay);
        
        const char* outputFilename = (argc >= 3) ? argv[argc - 1] : nullptr;
        bool readFromStdIn = (argc == 1);
        const char* inputFilename = (!readFromStdIn ? argv[1] : nullptr);
        
        return GlucoseArguments{
            verb,
            mod,
            cpu_lim,
            mem_lim,
            do_solve,
            do_preprocess,
            do_certified,
            do_gaterecognition,
            do_simp_out,
            do_minimize,
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
}
