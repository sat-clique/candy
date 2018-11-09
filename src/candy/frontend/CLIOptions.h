#ifndef CLI_PTIONS_H_
#define CLI_PTIONS_H_

#include "candy/utils/Options.h"

namespace Candy {

namespace ClauseLearningOptions {
	extern Glucose::IntOption opt_lb_size_minimzing_clause;
}

namespace ClauseDatabaseOptions {
    extern Glucose::IntOption opt_persistent_lbd;
    extern Glucose::DoubleOption opt_clause_decay;
}

namespace SolverOptions {
    using namespace Glucose;
    
    extern const char* _cat;
    extern const char* _cr;
    extern const char* _cred;
    extern const char* _cm;
    
    extern DoubleOption opt_K;
    extern DoubleOption opt_R;
    extern IntOption opt_size_lbd_queue;
    extern IntOption opt_size_trail_queue;
    
    extern IntOption opt_first_reduce_db;
    extern IntOption opt_inc_reduce_db;
    
    extern BoolOption opt_use_lrb;

    extern DoubleOption opt_var_decay;
    extern DoubleOption opt_max_var_decay;
    extern IntOption opt_phase_saving;
    
    extern IntOption opt_sonification_delay;
    
    extern BoolOption opt_sort_watches;
    extern BoolOption opt_sort_variables;
    extern IntOption opt_inprocessing;
}

};

#endif