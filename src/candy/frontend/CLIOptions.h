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
    extern BoolOption opt_reestimation_reduce_lbd;
}

namespace SolverOptions {
    extern IntOption verb;
    extern BoolOption mod;
    extern IntOption cpu_lim;
    extern IntOption mem_lim;
    extern BoolOption wait_for_user;    
    extern BoolOption do_solve;
    extern BoolOption do_certified;
    extern StringOption opt_certified_file;
    extern BoolOption do_gaterecognition;
    extern BoolOption do_simp_out;
    extern IntOption do_minimize;

    extern DoubleOption opt_K;
    extern DoubleOption opt_R;
    extern IntOption opt_size_lbd_queue;
    extern IntOption opt_size_trail_queue;
    
    extern IntOption opt_first_reduce_db;
    extern IntOption opt_inc_reduce_db;
    
    extern BoolOption opt_use_lrb; // lrb branching
    extern BoolOption opt_use_ts_ca; // thread-safe conflict analysis
    extern BoolOption opt_use_ts_pr; // thread-safe propagator

    extern DoubleOption opt_vsids_var_decay;
    extern DoubleOption opt_vsids_max_var_decay;
    extern BoolOption opt_vsids_extra_bump;

    extern IntOption opt_phase_saving;
    
    extern IntOption opt_sonification_delay;
    
    extern BoolOption opt_sort_watches;
    extern BoolOption opt_sort_variables;
    extern BoolOption opt_preprocessing;
    extern IntOption opt_inprocessing;
    extern DoubleOption opt_simplification_threshold_factor;
}

namespace VariableEliminationOptions {
    extern IntOption opt_clause_lim;
    extern IntOption opt_grow; 
    extern BoolOption opt_use_asymm;
    extern BoolOption opt_use_elim;
}

namespace SubsumptionOptions {
    extern IntOption opt_subsumption_lim;
}

namespace GateRecognitionOptions {
    extern BoolOption opt_print_gates;
    extern IntOption opt_gr_tries;
    extern BoolOption opt_gr_patterns;
    extern BoolOption opt_gr_semantic;
    extern IntOption opt_gr_semantic_budget;
    extern IntOption opt_gr_timeout;
    extern BoolOption opt_gr_holistic;
    extern BoolOption opt_gr_lookahead;
    extern IntOption opt_gr_lookahead_threshold;
    extern BoolOption opt_gr_intensify;
}

namespace RandomSimulationOptions {
    extern IntOption opt_rs_nrounds;
    extern BoolOption opt_rs_abortbyrrat;
    extern DoubleOption opt_rs_rrat;
    extern IntOption opt_rs_filterConjBySize;
    extern BoolOption opt_rs_removeBackboneConj;
    extern BoolOption opt_rs_filterGatesByNonmono;
    extern IntOption opt_rs_ppTimeLimit;
}

namespace RSILOptions {
    extern BoolOption opt_rsil_enable;
    extern StringOption opt_rsil_mode;
    extern IntOption opt_rsil_vanHalfLife;
    extern IntOption opt_rsil_impBudgets;
    extern IntOption opt_rsil_filterByInputDeps;
    extern BoolOption opt_rsil_filterOnlyBackbone;
    extern DoubleOption opt_rsil_minGateFraction;
    extern BoolOption opt_rsil_onlyMiters;
}

namespace RSAROptions {        
    extern BoolOption opt_rsar_enable;
    extern IntOption opt_rsar_maxRefinementSteps;
    extern StringOption opt_rsar_simpMode;
    extern StringOption opt_rsar_inputDepCountHeurConf;
    extern IntOption opt_rsar_minGateCount;
}

};

#endif