/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser and Felix Kutzner, KIT - Karlsruhe Institute of Technology

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

#include "candy/frontend/CLIOptions.h"

#include "candy/utils/Options.h"

namespace Candy {

namespace ParallelOptions {
    IntOption opt_threads("ParallelOptions", "threads", "Number of threads", 1, IntRange(1, 16));
    IntOption opt_thread_initialization_delay("ParallelOptions", "thread-initialization-delay", "Initialization delay for thread to increase agility and diversification (ms)", 10000, IntRange(10, INT32_MAX));
    BoolOption opt_static_propagate("ParallelOptions", "static-propagate", "use thread-safe propagation module", false);
    BoolOption opt_static_database("ParallelOptions", "static-database", "Use thread-safe static clause-allocator", false);
    IntOption opt_static_database_size_bound("ParallelOptions", "static_database_size_bound", "upper size-bound for static database (0 = disabled, 1+2 = no effect, 3++ = size-bound", 6, IntRange(0, INT16_MAX));
}

namespace ClauseDatabaseOptions {
    IntOption opt_persistent_lbd("ClauseDatabase", "persistentLBD", "Minimum LBD value for learnt clauses to be kept persistent", 3, IntRange(0, INT8_MAX));
    BoolOption opt_recalculate_lbd("ClauseDatabase", "recalculate-lbd", "After conflict recalculate lbd of involved clauses", true);
    BoolOption opt_keep_median_lbd("ClauseDatabase", "keep-median-lbd", "Delete only clauses with lbd > median(lbd)", false);

    IntOption opt_first_reduce_db("ClauseDatabase", "firstReduceDB", "The number of conflicts before the first reduce DB", 3000, IntRange(0, INT16_MAX));
    IntOption opt_inc_reduce_db("ClauseDatabase", "incReduceDB", "Increment for reduce DB", 1300, IntRange(0, INT16_MAX));
}

namespace MinimizerOptions {
    BoolOption do_minimize("Minimizer", "minimize", "Activate Model Minimization.", false);
    BoolOption minimize_pruned("Minimizer", "minimize-pruned", "Activate Pruning.", false);
    BoolOption minimize_minimal("Minimizer", "minimize-minimal", "Activate Minimal Model Computation.", false);
    BoolOption minimize_project("Minimizer", "minimize-project", "Project to input variables.", false);
}

namespace TestingOptions {
    BoolOption test_model("TEST", "test-model", "test model.", false);
    BoolOption test_proof("TEST", "test-proof", "test proof.", false);
    IntOption test_limit("TEST", "test-limit", "limit the number of variables ('0' means inactive).", 0, IntRange(0, 1000));
}

namespace SolverOptions {
    IntOption verb("MAIN", "verb", "Verbosity level (0=silent, 1=some, 2=more).", 1, IntRange(0, 2));
    BoolOption mod("MAIN", "model", "show model.", false);
    StringOption opt_certified_file("MAIN", "certified-output", "Certified UNSAT output file", "");
    BoolOption gate_stats("MAIN", "gate-stats", "show only gate recognizer statistics.", false);

    IntOption memory_limit("MAIN", "memory-limit", "Limit on memory usage in mega bytes.\n", INT32_MAX, IntRange(0, INT32_MAX));
    IntOption time_limit("MAIN", "time-limit", "Limit on wallclock runtime in seconds.\n", INT32_MAX, IntRange(0, INT32_MAX));
    
    DoubleOption opt_K("Restarts", "K", "The constant used to force restart", 0.8, DoubleRange(0, false, 1, false));
    DoubleOption opt_R("Restarts", "R", "The constant used to block restart", 1.4, DoubleRange(1, false, 5, false));
    IntOption opt_size_lbd_queue("Restarts", "szLBDQueue", "The size of moving average for LBD (restarts)", 50, IntRange(10, INT16_MAX));
    IntOption opt_size_trail_queue("Restarts", "szTrailQueue", "The size of moving average for trail (block restarts)", 5000, IntRange(10, INT16_MAX));

    DoubleOption opt_vsids_var_decay("CORE", "var-decay", "The variable activity decay factor (starting point)", 0.8, DoubleRange(0, false, 1, false));
    DoubleOption opt_vsids_max_var_decay("CORE", "max-var-decay", "The variable activity decay factor", 0.95, DoubleRange(0, false, 1, false));
    BoolOption opt_vsids_extra_bump("CORE", "extra-bump", "Glucose Style Extra Bumping on current desicion level", false);

    BoolOption opt_use_vsidsc("BRANCHING", "use-vsidsc", "use VSIDS branching heuristic with centrality (default: use VSIDS)", false);
    IntOption opt_vsidsc_mult("BRANCHING", "vsidsc-mult", "VSIDSC multiplier for centrality values", 1, IntRange(1, INT16_MAX));
    BoolOption opt_vsidsc_bump("BRANCHING", "vsidsc-bump", "use VSIDSC bumping based on centrality value", true);
    BoolOption opt_vsidsc_scope("BRANCHING", "vsidsc-lwcc", "VSIDSC uses LWCC graph scope (default: FULL)", false);
    DoubleOption opt_vsidsc_samplesize("BRANCHING", "vsidsc-sample-size", "VSIDSC sample size for centrality calculation (only for vsidsc-centralities 0, 1)", 1.0, DoubleRange(0.0, false, 1.0, true));
    DoubleOption opt_vsidsc_samplesize_decay("BRANCHING", "vsidsc-sample-size-decay", "VSIDSC decay sample size by factor for repeated centrality calculations (1=no decay)", 1.0, DoubleRange(0.0, false, 1.0, true));
    DoubleOption opt_vsidsc_dbsize_recalc("BRANCHING", "vsidsc-dbsize-recalc", "VSIDSC centrality recalc upon db_reduce if db_size grew by this factor since last calculation (1=every db_size increase, 0=once at start)", 0.0, DoubleRange(0.0, false, 10.0, true));
    BoolOption opt_vsidsc_debug("BRANCHING", "vsidsc-debug", "Active Centrality debugging", false);

    BoolOption opt_use_lrb("BRANCHING", "use-lrb", "use LRB branching heuristic (default: use VSIDS)", false);
    DoubleOption opt_lrb_step_size("CORE", "lrb-step-size", "The lrb step size (starting point)", 0.4, DoubleRange(0, false, 1, false));
    DoubleOption opt_lrb_min_step_size("CORE", "lrb-min-step-size", "The lrb minimium step size", 0.06, DoubleRange(0, false, 1, false));

    BoolOption opt_sort_variables("MEMORY LAYOUT", "sort-variables", "sort variables", false);
    BoolOption opt_preprocessing("METHOD", "pre", "Completely turn on/off any preprocessing.", true);
    IntOption opt_inprocessing("MEMORY LAYOUT", "inprocessing", "execute eliminate with persistent clauses during search every n-th restart", 0);
}

namespace SonificationOptions {
    StringOption host("SONIFICATION", "sonification-host", "host of sound synthesizer (for osc messages)", "127.0.0.1");
    IntOption port("SONIFICATION", "sonification-port", "port of sound synthesizer (for osc messages)", 7000, IntRange(1, 99999));
    IntOption delay("SONIFICATION", "sonification-delay", "minimum delay (ms) between timing events to reduce number of events per second", 0, IntRange(0, INT16_MAX));
}

namespace VariableEliminationOptions {
    IntOption opt_clause_lim("VariableElimination", "cl-lim", "Variables are not eliminated if it produces a resolvent with a length above this limit.", 20, IntRange(0, INT32_MAX));
    BoolOption opt_use_asymm("VariableElimination", "asymm", "Shrink clauses by asymmetric branching.", false, true); // disabled option
    BoolOption opt_use_elim("VariableElimination", "elim", "Perform variable elimination.", true);
}

namespace GateRecognitionOptions {
    IntOption method("GATE RECOGNITION", "gate-recognition-method", "0 = patterns, 1 = semantic, 2 = holistic,\n 10 = patterns, semantic, 11 = patterns, semantic, holistic", 10, IntRange(0, 30));
    IntOption tries("GATE RECOGNITION", "gate-tries", "Number of heuristic clause selections to enter recursion (0 -> unlimited)", 0, IntRange(0, INT32_MAX)); 
    IntOption timeout("GATE RECOGNITION", "gate-timeout", "Enable Gate Detection Timeout (seconds)", 0, IntRange(0, INT32_MAX));
}

namespace RandomSimulationOptions {
    IntOption opt_rs_nrounds("RANDOMSIMULATION", "rs-rounds", "Amount of random simulation rounds (gets rounded up to the next multiple of 2048)", 1048576, IntRange(1, INT32_MAX));
    BoolOption opt_rs_abortbyrrat("RANDOMSIMULATION", "rs-abort-by-rrat", "Abort random simulation when the reduction rate falls below the RRAT threshold", false);
    DoubleOption opt_rs_rrat("RANDOMSIMULATION", "rs-rrat", "Reduction rate abort threshold", 0.01, DoubleRange(0.0, true, 1.0, false));
    IntOption opt_rs_filterConjBySize("RANDOMSIMULATION", "rs-max-conj-size", "Max. allowed literal equivalence conjecture size (0: disable filtering by size)", 0, IntRange(0, INT32_MAX));
    BoolOption opt_rs_removeBackboneConj("RANDOMSIMULATION", "rs-remove-backbone-conj", "Filter out conjectures about the problem's backbone", false);
    BoolOption opt_rs_filterGatesByNonmono("RANDOMSIMULATION", "rs-only-nonmono-gates", "Use only non-monotonic nested gates for random simulation", false);
    IntOption opt_rs_ppTimeLimit("RANDOMSIMULATION", "rs-time-limit", "Time limit for preprocessing (gate recognition + rs; -1 to disable; default: disabled)", -1, IntRange(-1, INT32_MAX));
}

namespace RSILOptions {
    BoolOption opt_rsil_enable("RSIL", "rsil-enable", "Enable random-simulation-based implicit learning heuristics", false);
    StringOption opt_rsil_mode("RSIL", "rsil-mode", "Set RSIL mode to unrestricted, vanishing or implicationbudgeted", "unrestricted");
    IntOption opt_rsil_advice_size("RSIL", "advice-size", "Set RSIL advice size", 3, IntRange(2, 3));
    IntOption opt_rsil_vanHalfLife("RSIL", "rsil-van-halflife", "Set the probability half-life (in decisions) for vanishing mode", 1 << 24, IntRange(1, INT32_MAX));
    IntOption opt_rsil_impBudgets("RSIL", "rsil-imp-budgets", "Set the initial budgets for implicationbudgeted mode", 1 << 20, IntRange(1, INT32_MAX));
    IntOption opt_rsil_filterByInputDeps("RSIL", "rsil-filter-by-input-dependencies", "Disregard variables dependending on more than N inputs. N=0 (default) disables this filter.", 
        0, IntRange(0, INT32_MAX));
    BoolOption opt_rsil_filterOnlyBackbone("RSIL", "rsil-filter-only-backbone", "Filter only the backbone of the problem via rsil-filter-by-input-dependencies", false);
    DoubleOption opt_rsil_minGateFraction("RSIL", "rsil-min-gate-frac", "Enable RSIL only when at least this fraction of variables consists of gate outputs", 0.1, DoubleRange(0.0, true, 1.0, true));
    BoolOption opt_rsil_onlyMiters("RSIL", "rsil-only-miters", "Enable RSIL only for miter problems (heuristic detection)", false);
}

namespace RSAROptions {        
    BoolOption opt_rsar_enable("RSAR", "rsar-enable", "Enable random-simulation-based abstraction refinement SAT solving", false);
    IntOption opt_rsar_maxRefinementSteps("RSAR", "rsar-max-refinements", "Max. refinement steps", 10, IntRange(1, INT32_MAX));
    StringOption opt_rsar_inputDepCountHeurConf("RSAR", "rsar-heur-idc", "Input dependency count heuristic configuration", "");
    IntOption opt_rsar_minGateCount("RSAR", "rsar-min-gatecount", "Minimum amount of recognized gates for RSAR to be enabled", 100, IntRange(1, INT32_MAX));
}

}