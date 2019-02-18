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
    IntOption opt_threads("ParallelOptions", "threads", "Number of threads", 1, IntRange(1, 8));
    BoolOption opt_static_propagate("ParallelOptions", "static-propagate", "use thread-safe propagation module", false);
    BoolOption opt_static_database("ParallelOptions", "static-database", "Use thread-safe static clause-allocator", false);
    IntOption opt_static_database_size_bound("ParallelOptions", "static_database_size_bound", "upper size-bound for static database", 6, IntRange(3, INT16_MAX));
}

namespace ClauseDatabaseOptions {
    IntOption opt_persistent_lbd("ClauseDatabase", "persistentLBD", "Minimum LBD value for learnt clauses to be kept persistent", 3, IntRange(0, INT8_MAX));
    BoolOption opt_reestimation_reduce_lbd("ClauseDatabase", "reestimate-lbd", "After conflict reduce lbds", true);
    BoolOption opt_keep_median_lbd("ClauseDatabase", "keep-median-lbd", "Delete only clauses with lbd > median(lbd)", false);
}

namespace SolverOptions {
    IntOption verb("MAIN", "verb", "Verbosity level (0=silent, 1=some, 2=more).", 1, IntRange(0, 2));
    BoolOption mod("MAIN", "model", "show model.", false);
    
    IntOption cpu_lim("MAIN", "cpu-lim", "Limit on CPU time allowed in seconds.\n", INT32_MAX, IntRange(0, INT32_MAX));
    
    BoolOption do_solve("METHOD", "solve", "Completely turn on/off actual sat solving.", true);
    StringOption opt_certified_file("METHOD", "certified-output", "Certified UNSAT output file", "");
    BoolOption do_gaterecognition("METHOD", "gates", "Completely turn on/off actual gate recognition.", false);
    IntOption do_minimize("METHOD", "minimize", "Model Minimization (0=none, 1=normal, 2=pruning).", 0, IntRange(0, 2));
    
    DoubleOption opt_K("Restarts", "K", "The constant used to force restart", 0.8, DoubleRange(0, false, 1, false));
    DoubleOption opt_R("Restarts", "R", "The constant used to block restart", 1.4, DoubleRange(1, false, 5, false));
    IntOption opt_size_lbd_queue("Restarts", "szLBDQueue", "The size of moving average for LBD (restarts)", 50, IntRange(10, INT16_MAX));
    IntOption opt_size_trail_queue("Restarts", "szTrailQueue", "The size of moving average for trail (block restarts)", 5000, IntRange(10, INT16_MAX));

    IntOption opt_first_reduce_db("DB Reduction", "firstReduceDB", "The number of conflicts before the first reduce DB", 3000, IntRange(0, INT16_MAX));
    IntOption opt_inc_reduce_db("DB Reduction", "incReduceDB", "Increment for reduce DB", 1300, IntRange(0, INT16_MAX));

    DoubleOption opt_vsids_var_decay("CORE", "var-decay", "The variable activity decay factor (starting point)", 0.8, DoubleRange(0, false, 1, false));
    DoubleOption opt_vsids_max_var_decay("CORE", "max-var-decay", "The variable activity decay factor", 0.95, DoubleRange(0, false, 1, false));
    BoolOption opt_vsids_extra_bump("CORE", "extra-bump", "Glucose Style Extra Bumping on current desicion level", false);

    BoolOption opt_use_lrb("BRANCHING", "use-lrb", "use LRB branching heuristic (default: use VSIDS)", false);

    IntOption opt_sonification_delay("SONIFICATION", "sonification-delay", "ms delay after each event to improve realtime sonification", 0, IntRange(0, INT16_MAX));

    BoolOption opt_sort_watches("MEMORY LAYOUT", "sort-watches", "sort watches", true);
    BoolOption opt_sort_variables("MEMORY LAYOUT", "sort-variables", "sort variables", true);
    BoolOption opt_preprocessing("METHOD", "pre", "Completely turn on/off any preprocessing.", true);
    IntOption opt_inprocessing("MEMORY LAYOUT", "inprocessing", "execute eliminate with persistent clauses during search every n-th restart", 0);
    IntOption opt_unitresolution("MEMORY LAYOUT", "unitresolution", "execute unitresolution with persistent clauses during search every n-th restart", 1);
    DoubleOption opt_simplification_threshold_factor("MEMORY LAYOUT", "simplification-threshold-factor", "Simplification Threshold Factor", 0.1, DoubleRange(0, false, 1, false));
}

namespace VariableEliminationOptions {
    IntOption opt_grow("VariableElimination", "grow", "Allow a variable elimination step to grow by a number of clauses.", 0);
    IntOption opt_clause_lim("VariableElimination", "cl-lim", "Variables are not eliminated if it produces a resolvent with a length above this limit.", 20, IntRange(0, INT32_MAX));
    BoolOption opt_use_asymm("VariableElimination", "asymm", "Shrink clauses by asymmetric branching.", false);
    BoolOption opt_use_elim("VariableElimination", "elim", "Perform variable elimination.", true);
}

namespace SubsumptionOptions {
    IntOption opt_subsumption_lim("SIMP", "sub-lim", "Do not check for subsumption with a clause larger than this.", 100, IntRange(0, INT32_MAX));
}

namespace GateRecognitionOptions {
    BoolOption opt_print_gates("GATE RECOGNITION", "print-gates", "print gates.", false);
    IntOption opt_gr_tries("GATE RECOGNITION", "gate-tries", "Number of heuristic clause selections to enter recursion", 3, IntRange(0, INT32_MAX)); 
    BoolOption opt_gr_patterns("GATE RECOGNITION", "gate-patterns", "Enable Pattern-based Gate Detection", true);
    BoolOption opt_gr_semantic("GATE RECOGNITION", "gate-semantic", "Enable Semantic Gate Detection", false);
    IntOption opt_gr_semantic_budget("GATE RECOGNITION", "gate-semantic-budget", "Enable Semantic Gate Detection Conflict Budget", 0, IntRange(0, INT32_MAX));
    IntOption opt_gr_timeout("GATE RECOGNITION", "gate-timeout", "Enable Gate Detection Timeout (seconds)", 0, IntRange(0, INT32_MAX));
    BoolOption opt_gr_holistic("GATE RECOGNITION", "gate-holistic", "Enable Holistic Gate Detection", false);
    BoolOption opt_gr_lookahead("GATE RECOGNITION", "gate-lookahead", "Enable Local Blocked Elimination", false);
    IntOption opt_gr_lookahead_threshold("GATE RECOGNITION", "gate-lookahead-threshold", "Local Blocked Elimination Threshold", 10, IntRange(1, INT32_MAX));
    BoolOption opt_gr_intensify("GATE RECOGNITION", "gate-intensification", "Enable Intensification", true);
}

namespace RandomSimulationOptions {
    IntOption opt_rs_nrounds("RANDOMSIMULATION", "rs-rounds", "Amount of random simulation rounds (gets rounded up to the next multiple of 2048)", 1048576, IntRange(1, INT32_MAX));
    BoolOption opt_rs_abortbyrrat("RANDOMSIMULATION", "rs-abort-by-rrat", "Abort random simulation when the reduction rate falls below the RRAT threshold", false);
    DoubleOption opt_rs_rrat("RANDOMSIMULATION", "rs-rrat", "Reduction rate abort threshold", 0.01, DoubleRange(0.0, true, 1.0, false));
    IntOption opt_rs_filterConjBySize("RANDOMSIMULATION", "rs-max-conj-size", "Max. allowed literal equivalence conjecture size (0: disable filtering by size)", 0, IntRange(0, INT32_MAX));
    BoolOption opt_rs_removeBackboneConj("RANDOMSIMULATION", "rs-remove-backbone-conj", "Filter out conjectures about the problem's backbone", false);
    BoolOption opt_rs_filterGatesByNonmono("RANDOMSIMULATION", "rs-only-nonmono-gates", "Use only nonmonotonously nested gates for random simulation", false);
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