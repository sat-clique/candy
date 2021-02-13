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

#include "candy/utils/CLIOptions.h"

#include "candy/utils/Options.h"

namespace Candy {

namespace ParallelOptions {
    IntOption opt_threads("ParallelOptions", "threads", "Number of threads", 1, IntRange(1, 16));
    IntOption opt_thread_initialization_delay("ParallelOptions", "thread-initialization-delay", "Initialization delay for thread to increase agility and diversification (ms)", 10000, IntRange(10, INT32_MAX));
    BoolOption opt_static_propagate("ParallelOptions", "static-propagate", "use static two-w.l. propagation module", false);
    BoolOption opt_3full_propagate("ParallelOptions", "3full-propagate", "use ternary full-ol propagation module", false);
    IntOption opt_Xfull_propagate("ParallelOptions", "Xfull-propagate", "use X-ary full-ol propagation module", 2, IntRange(2, 5));
    BoolOption opt_lb_propagate("ParallelOptions", "lb-propagate", "use static lower-bounds propagation module", false);
    BoolOption opt_static_database("ParallelOptions", "static-database", "Use thread-safe static clause-allocator", false);
    IntOption opt_static_database_size_bound("ParallelOptions", "static_database_size_bound", "upper size-bound for static database (0 = disabled, 1+2 = no effect, 3++ = size-bound", 6, IntRange(0, INT16_MAX));
}

namespace ClauseDatabaseOptions {
    IntOption opt_persistent_lbd("ClauseDatabase", "persistentLBD", "Start of second tier of learnt clause", 3, IntRange(0, INT8_MAX));
    IntOption opt_volatile_lbd("ClauseDatabase", "volatileLBD", "Start of third tier of learnt clauses", 6, IntRange(0, INT8_MAX));

    IntOption opt_first_reduce_db("ClauseDatabase", "firstReduceDB", "The number of conflicts before the first reduce DB", 3000, IntRange(0, INT16_MAX));
    IntOption opt_inc_reduce_db("ClauseDatabase", "incReduceDB", "Increment for reduce DB", 1300, IntRange(0, INT16_MAX));
}

namespace TestingOptions {
    BoolOption test_model("TEST", "test-model", "test model.", false);
    BoolOption test_proof("TEST", "test-proof", "test proof.", false);
    IntOption test_limit("TEST", "test-limit", "limit the number of variables ('0' means inactive).", 0, IntRange(0, 1000));
}

namespace LearningOptions {
    IntOption equiv("Learning", "equiv", "Explicit greedy handling of equivalences", 0, IntRange(0, INT16_MAX));
}

namespace SolverOptions {
    IntOption verb("MAIN", "verb", "Verbosity level (0=silent, 1=some, 2=more).", 1, IntRange(0, 2));
    BoolOption mod("MAIN", "model", "show model.", false);
    StringOption opt_certified_file("MAIN", "certified-output", "Certified UNSAT output file", "");
    BoolOption gate_stats("MAIN", "gate-stats", "show only gate recognizer statistics.", false);

    IntOption memory_limit("MAIN", "memory-limit", "Limit on memory usage in mega bytes.\n", INT32_MAX, IntRange(0, INT32_MAX));
    IntOption time_limit("MAIN", "time-limit", "Limit on wallclock runtime in seconds.\n", INT32_MAX, IntRange(0, INT32_MAX));
    
    DoubleOption opt_restart_force("Restarts", "restart-force", "The constant used to force restart", 1.3, DoubleRange(1, false, 5, false));
    DoubleOption opt_restart_block("Restarts", "restart-block", "The constant used to block restart", 1.3, DoubleRange(1, false, 5, false));
    IntOption opt_size_lbd_queue("Restarts", "szLBDQueue", "The size of moving average for LBD (restarts)", 50, IntRange(10, INT16_MAX));
    IntOption opt_size_trail_queue("Restarts", "szTrailQueue", "The size of moving average for trail (block restarts)", 5000, IntRange(10, INT16_MAX));

    DoubleOption opt_vsids_var_decay("BRANCHING", "var-decay", "The variable activity decay factor (starting point)", 0.8, DoubleRange(0, false, 1, false));
    DoubleOption opt_vsids_max_var_decay("BRANCHING", "max-var-decay", "The variable activity decay factor", 0.95, DoubleRange(0, false, 1, false));

    BoolOption opt_use_lrb("BRANCHING", "use-lrb", "use LRB branching heuristic (default: use VSIDS)", false);
    DoubleOption opt_lrb_step_size("BRANCHING", "lrb-step-size", "The lrb step size (starting point)", 0.4, DoubleRange(0, false, 1, false));
    DoubleOption opt_lrb_min_step_size("BRANCHING", "lrb-min-step-size", "The lrb minimium step size", 0.06, DoubleRange(0, false, 1, false));

    IntOption opt_sort_variables("BRANCHING", "sort-variables", "sort variables", 0, IntRange(0,3));

    BoolOption opt_preprocessing("METHOD", "pre", "Completely turn on/off any preprocessing.", true);
    IntOption opt_inprocessing("METHOD", "inprocessing", "execute eliminate with persistent clauses during search every n-th restart", 0);
}

namespace VariableEliminationOptions {
    IntOption opt_clause_lim("VariableElimination", "cl-lim", "Variables are not eliminated if it produces a resolvent with a length above this limit.", 20, IntRange(0, INT32_MAX));
    BoolOption opt_use_elim("VariableElimination", "elim", "Perform variable elimination.", true);
}

}