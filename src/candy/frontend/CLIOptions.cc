#include "candy/frontend/CLIOptions.h"

#include "candy/utils/Options.h"

namespace Candy {

namespace ClauseLearningOptions {
	IntOption opt_lb_size_minimzing_clause("ClauseLearning", "minSizeMinimizingClause", "The min size required to minimize clause", 30, IntRange(3, INT16_MAX));
}

namespace ClauseDatabaseOptions {
    IntOption opt_persistent_lbd("ClauseDatabase", "persistentLBD", "Minimum LBD value for learnt clauses to be kept persistent", 3, IntRange(0, INT8_MAX));
    DoubleOption opt_clause_decay("ClauseDatabase", "cla-decay", "The clause activity decay factor", 0.999, DoubleRange(0, false, 1, false));
}

namespace SolverOptions {
    DoubleOption opt_K("Restarts", "K", "The constant used to force restart", 0.8, DoubleRange(0, false, 1, false));
    DoubleOption opt_R("Restarts", "R", "The constant used to block restart", 1.4, DoubleRange(1, false, 5, false));
    IntOption opt_size_lbd_queue("Restarts", "szLBDQueue", "The size of moving average for LBD (restarts)", 50, IntRange(10, INT16_MAX));
    IntOption opt_size_trail_queue("Restarts", "szTrailQueue", "The size of moving average for trail (block restarts)", 5000, IntRange(10, INT16_MAX));

    IntOption opt_first_reduce_db("DB Reduction", "firstReduceDB", "The number of conflicts before the first reduce DB", 3000, IntRange(0, INT16_MAX));
    IntOption opt_inc_reduce_db("DB Reduction", "incReduceDB", "Increment for reduce DB", 1300, IntRange(0, INT16_MAX));

    DoubleOption opt_var_decay("CORE", "var-decay", "The variable activity decay factor (starting point)", 0.8, DoubleRange(0, false, 1, false));
    DoubleOption opt_max_var_decay("CORE", "max-var-decay", "The variable activity decay factor", 0.95, DoubleRange(0, false, 1, false));

    BoolOption opt_use_lrb("MEMORY LAYOUT", "use-lrb", "use LRB branching heuristic (default: use VSIDS)", false);

    IntOption opt_sonification_delay("SONIFICATION", "sonification-delay", "ms delay after each event to improve realtime sonification", 0, IntRange(0, INT16_MAX));

    BoolOption opt_sort_watches("MEMORY LAYOUT", "sort-watches", "sort watches", true);
    BoolOption opt_sort_variables("MEMORY LAYOUT", "sort-variables", "sort variables", true);
    IntOption opt_inprocessing("MEMORY LAYOUT", "inprocessing", "execute eliminate with persistent clauses during search every n-th restart", 0);
}

namespace SimpSolverOptions {
    BoolOption opt_use_asymm("SIMP", "asymm", "Shrink clauses by asymmetric branching.", false);
    BoolOption opt_use_rcheck("SIMP", "rcheck", "Check if a clause is already implied. (costly)", false);
    BoolOption opt_use_elim("SIMP", "elim", "Perform variable elimination.", true);
}

namespace VariableEliminationOptions {
    IntOption opt_grow("VariableElimination", "grow", "Allow a variable elimination step to grow by a number of clauses.", 0);
    IntOption opt_clause_lim("VariableElimination", "cl-lim", "Variables are not eliminated if it produces a resolvent with a length above this limit.", 20, IntRange(0, INT32_MAX));
}

namespace SubsumptionOptions {
    IntOption opt_subsumption_lim("SIMP", "sub-lim", "Do not check if subsumption against a clause larger than this.", 1000, IntRange(0, INT32_MAX));
}

}