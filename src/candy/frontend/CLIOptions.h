#ifndef CLI_PTIONS_H_
#define CLI_PTIONS_H_

#include "candy/utils/Options.h"

namespace Candy {

namespace ClauseLearningOptions {
	extern IntOption opt_lb_size_minimzing_clause;
}

namespace ClauseDatabaseOptions {
    extern IntOption opt_persistent_lbd;
    extern DoubleOption opt_clause_decay;
}

namespace SolverOptions {
    extern DoubleOption opt_K;
    extern DoubleOption opt_R;
    extern IntOption opt_size_lbd_queue;
    extern IntOption opt_size_trail_queue;
    
    extern IntOption opt_first_reduce_db;
    extern IntOption opt_inc_reduce_db;
    
    extern BoolOption opt_use_lrb; // lrb branching
    extern BoolOption opt_use_ts_ca; // thread-safe conflict analysis
    extern BoolOption opt_use_ts_pr; // thread-safe propagator

    extern DoubleOption opt_var_decay;
    extern DoubleOption opt_max_var_decay;
    extern IntOption opt_phase_saving;
    
    extern IntOption opt_sonification_delay;
    
    extern BoolOption opt_sort_watches;
    extern BoolOption opt_sort_variables;
    extern IntOption opt_inprocessing;
}

namespace SimpSolverOptions {
    extern BoolOption opt_use_asymm;
    extern BoolOption opt_use_rcheck;
    extern BoolOption opt_use_elim;
}

namespace VariableEliminationOptions {
    extern IntOption opt_clause_lim;
    extern IntOption opt_grow; 
}

namespace SubsumptionOptions {
    extern IntOption opt_subsumption_lim;
}

};

#endif