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

#ifndef CLI_PTIONS_H_
#define CLI_PTIONS_H_

#include "candy/utils/Options.h"

namespace Candy {

namespace ParallelOptions {
    extern IntOption opt_threads;
    extern IntOption opt_thread_initialization_delay;
    extern BoolOption opt_static_propagate; // thread-safe propagator
    extern BoolOption opt_static_database;
    extern IntOption opt_static_database_size_bound;
}

namespace ClauseDatabaseOptions {
    extern IntOption opt_persistent_lbd;
    extern BoolOption opt_recalculate_lbd;
    extern BoolOption opt_keep_median_lbd;
    
    extern IntOption opt_first_reduce_db;
    extern IntOption opt_inc_reduce_db;
}

namespace MinimizerOptions {
    extern BoolOption do_minimize;
    extern BoolOption minimize_pruned;
    extern BoolOption minimize_minimal;
    extern BoolOption minimize_project;
}

namespace TestingOptions {
    extern BoolOption test_model;
    extern BoolOption test_proof;
    extern IntOption test_limit;
}

namespace SolverOptions {
    extern IntOption verb;
    extern BoolOption mod;
    extern StringOption opt_certified_file;
    extern BoolOption gate_stats;

    extern IntOption memory_limit;
    extern IntOption time_limit;

    extern DoubleOption opt_K;
    extern DoubleOption opt_R;
    extern IntOption opt_size_lbd_queue;
    extern IntOption opt_size_trail_queue;
    
    extern BoolOption opt_use_lrb; // lrb branching
    extern DoubleOption opt_lrb_step_size;
    extern DoubleOption opt_lrb_min_step_size;

    extern DoubleOption opt_vsids_var_decay;
    extern DoubleOption opt_vsids_max_var_decay;
    extern BoolOption opt_vsids_extra_bump;

    extern BoolOption opt_use_vsidsc; // vsids with centrality
    extern IntOption opt_vsidsc_mult; // centrality mult factor [1..]
    extern BoolOption opt_vsidsc_bump; // bump based on centrality
    extern BoolOption opt_vsidsc_scope; // centrality calc: scope for graph FULL / LWCC
    extern DoubleOption opt_vsidsc_samplesize; // centrality calc: samplesize ]0,1.0]
    extern DoubleOption opt_vsidsc_samplesize_decay;
    extern DoubleOption opt_vsidsc_dbsize_recalc;
    extern BoolOption opt_vsidsc_debug; // Centrality Debugging
    
    extern BoolOption opt_sort_variables;
    extern BoolOption opt_preprocessing;
    extern IntOption opt_inprocessing;
}

namespace SonificationOptions {
    extern StringOption host;
    extern IntOption port;
    extern IntOption delay;
}

namespace VariableEliminationOptions {
    extern IntOption opt_clause_lim;
    extern IntOption opt_grow; 
    extern BoolOption opt_use_asymm;
    extern BoolOption opt_use_elim;
}

namespace GateRecognitionOptions {
    extern IntOption method;
    extern IntOption tries;
    extern IntOption timeout;
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
    extern IntOption opt_rsil_advice_size;
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
    extern StringOption opt_rsar_inputDepCountHeurConf;
    extern IntOption opt_rsar_minGateCount;
}

};

#endif