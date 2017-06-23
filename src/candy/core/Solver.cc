/***************************************************************************************[Solver.cc]
 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 LRI  - Univ. Paris Sud, France (2009-2013)
 Labri - Univ. Bordeaux, France

 Syrup (Glucose Parallel) -- Copyright (c) 2013-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 Labri - Univ. Bordeaux, France

 Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
 Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it
 is based on. (see below).

 Glucose-Syrup sources are based on another copyright. Permissions and copyrights for the parallel
 version of Glucose-Syrup (the "Software") are granted, free of charge, to deal with the Software
 without restriction, including the rights to use, copy, modify, merge, publish, distribute,
 sublicence, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 - The above and below copyrights notices and this permission notice shall be included in all
 copies or substantial portions of the Software;
 - The parallel version of Glucose (all files modified since Glucose 3.0 releases, 2013) cannot
 be used in any competitive event (sat competitions/evaluations) without the express permission of
 the authors (Gilles Audemard / Laurent Simon). This is also the case for any competitive event
 using Glucose Parallel as an embedded SAT engine (single core or not).


 --------------- Original Minisat Copyrights

 Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 Copyright (c) 2007-2010, Niklas Sorensson

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

#include <math.h>
#include <string>
#include <candy/core/Solver.h>
#include <candy/utils/System.h>

using namespace Glucose;
using namespace Candy;

//=================================================================================================
// Options:

namespace Candy {
namespace SolverOptions {

const char* _cat = "CORE";
const char* _cr = "CORE -- RESTART";
const char* _cred = "CORE -- REDUCE";
const char* _cm = "CORE -- MINIMIZE";
    
DoubleOption opt_K(_cr, "K", "The constant used to force restart", 0.8, DoubleRange(0, false, 1, false));
DoubleOption opt_R(_cr, "R", "The constant used to block restart", 1.4, DoubleRange(1, false, 5, false));
IntOption opt_size_lbd_queue(_cr, "szLBDQueue", "The size of moving average for LBD (restarts)", 50, IntRange(10, INT16_MAX));
IntOption opt_size_trail_queue(_cr, "szTrailQueue", "The size of moving average for trail (block restarts)", 5000, IntRange(10, INT16_MAX));

IntOption opt_first_reduce_db(_cred, "firstReduceDB", "The number of conflicts before the first reduce DB", 3000, IntRange(0, INT16_MAX));
IntOption opt_inc_reduce_db(_cred, "incReduceDB", "Increment for reduce DB", 1300, IntRange(0, INT16_MAX));
IntOption opt_persistent_lbd(_cred, "persistentLBD", "Minimum LBD value for learnt clauses to be kept persistent", 3, IntRange(0, INT16_MAX));
IntOption opt_lb_lbd_frozen_clause(_cred, "minLBDFrozenClause", "Protect clauses if their LBD decrease and is lower than (for one turn)", 30, IntRange(0, INT16_MAX));

IntOption opt_lb_size_minimzing_clause(_cm, "minSizeMinimizingClause", "The min size required to minimize clause", 30, IntRange(3, INT16_MAX));
IntOption opt_lb_lbd_minimzing_clause(_cm, "minLBDMinimizingClause", "The min LBD required to minimize clause", 6, IntRange(3, INT16_MAX));

DoubleOption opt_var_decay(_cat, "var-decay", "The variable activity decay factor (starting point)", 0.8, DoubleRange(0, false, 1, false));
DoubleOption opt_max_var_decay(_cat, "max-var-decay", "The variable activity decay factor", 0.95, DoubleRange(0, false, 1, false));
DoubleOption opt_clause_decay(_cat, "cla-decay", "The clause activity decay factor", 0.999, DoubleRange(0, false, 1, false));

IntOption opt_sonification_delay("SONIFICATION", "sonification-delay", "ms delay after each event to improve realtime sonification", 0, IntRange(0, INT16_MAX));

IntOption opt_revamp("MEMORY LAYOUT", "revamp", "reorganize memory to keep active clauses close", 6, IntRange(2, REVAMPABLE_PAGES_MAX_SIZE));
BoolOption opt_sort_watches("MEMORY LAYOUT", "sort-watches", "sort watches", true);
BoolOption opt_sort_variables("MEMORY LAYOUT", "sort-variables", "sort variables", true);
IntOption opt_inprocessing("MEMORY LAYOUT", "inprocessing", "execute eliminate with persistent clauses during search every n-th restart", 0);
    
}

template <>
Lit Solver<DefaultPickBranchLit>::pickBranchLit() {
    return defaultPickBranchLit();
}
}

//=================================================================================================
// Constructor/Destructor:

