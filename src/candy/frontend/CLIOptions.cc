#include "candy/frontend/CLIOptions.h"

namespace Candy {

namespace ClauseLearningOptions {
	Glucose::IntOption opt_lb_size_minimzing_clause("ClauseLearning", "minSizeMinimizingClause", "The min size required to minimize clause", 30, Glucose::IntRange(3, INT16_MAX));
}

namespace ClauseDatabaseOptions {
    Glucose::IntOption opt_persistent_lbd("ClauseDatabase", "persistentLBD", "Minimum LBD value for learnt clauses to be kept persistent", 3, Glucose::IntRange(0, INT8_MAX));
    Glucose::DoubleOption opt_clause_decay("ClauseDatabase", "cla-decay", "The clause activity decay factor", 0.999, Glucose::DoubleRange(0, false, 1, false));
}

namespace SolverOptions {
    Glucose::DoubleOption opt_K("Restarts", "K", "The constant used to force restart", 0.8, DoubleRange(0, false, 1, false));
    Glucose::DoubleOption opt_R("Restarts", "R", "The constant used to block restart", 1.4, DoubleRange(1, false, 5, false));
    Glucose::IntOption opt_size_lbd_queue("Restarts", "szLBDQueue", "The size of moving average for LBD (restarts)", 50, IntRange(10, INT16_MAX));
    Glucose::IntOption opt_size_trail_queue("Restarts", "szTrailQueue", "The size of moving average for trail (block restarts)", 5000, IntRange(10, INT16_MAX));

    Glucose::IntOption opt_first_reduce_db("DB Reduction", "firstReduceDB", "The number of conflicts before the first reduce DB", 3000, IntRange(0, INT16_MAX));
    Glucose::IntOption opt_inc_reduce_db("DB Reduction", "incReduceDB", "Increment for reduce DB", 1300, IntRange(0, INT16_MAX));

    Glucose::DoubleOption opt_var_decay("CORE", "var-decay", "The variable activity decay factor (starting point)", 0.8, DoubleRange(0, false, 1, false));
    Glucose::DoubleOption opt_max_var_decay("CORE", "max-var-decay", "The variable activity decay factor", 0.95, DoubleRange(0, false, 1, false));

    Glucose::BoolOption opt_use_lrb("MEMORY LAYOUT", "use-lrb", "use LRB branching heuristic (default: use VSIDS)", false);

    Glucose::IntOption opt_sonification_delay("SONIFICATION", "sonification-delay", "ms delay after each event to improve realtime sonification", 0, IntRange(0, INT16_MAX));

    Glucose::BoolOption opt_sort_watches("MEMORY LAYOUT", "sort-watches", "sort watches", true);
    Glucose::BoolOption opt_sort_variables("MEMORY LAYOUT", "sort-variables", "sort variables", true);
    Glucose::IntOption opt_inprocessing("MEMORY LAYOUT", "inprocessing", "execute eliminate with persistent clauses during search every n-th restart", 0);
}

}