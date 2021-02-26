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
    extern BoolOption opt_lb_propagate; // thread-safe propagator
    extern BoolOption opt_3full_propagate; // ternary clauses full
    extern IntOption opt_Xfull_propagate; // X-Z clauses full
    extern BoolOption opt_static_database;
    extern IntOption opt_static_database_size_bound;
}

namespace Stability {
    extern BoolOption opt_sort_by_stability; // watch most stable literals
    extern BoolOption opt_prop_by_stability; // stable one-watched
    extern BoolOption opt_reset_stability; // reset stability after reattach
}

namespace ClauseDatabaseOptions {
    extern IntOption opt_persistent_lbd;
    extern IntOption opt_volatile_lbd;
    
    extern IntOption opt_first_reduce_db;
    extern IntOption opt_inc_reduce_db;
}

namespace TestingOptions {
    extern BoolOption test_model;
    extern BoolOption test_proof;
    extern IntOption test_limit;
}

namespace LearningOptions {
    extern IntOption equiv;
}

namespace SolverOptions {
    extern IntOption verb;
    extern BoolOption mod;
    extern StringOption opt_certified_file;
    extern BoolOption gate_stats;

    extern IntOption memory_limit;
    extern IntOption time_limit;

    extern DoubleOption opt_restart_force;
    extern DoubleOption opt_restart_block;
    extern IntOption opt_size_lbd_queue;
    extern IntOption opt_size_trail_queue;
    
    extern BoolOption opt_use_lrb; // lrb branching
    extern DoubleOption opt_lrb_step_size;
    extern DoubleOption opt_lrb_min_step_size;

    extern DoubleOption opt_vsids_var_decay;
    extern DoubleOption opt_vsids_max_var_decay;
    
    extern IntOption opt_sort_variables;
    extern BoolOption opt_sort_clauses;

    extern BoolOption opt_preprocessing;
    extern IntOption opt_inprocessing;
}

namespace VariableEliminationOptions {
    extern IntOption opt_clause_lim;
    extern BoolOption opt_use_elim;
}

};

#endif