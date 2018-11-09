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
#include <candy/rsil/BranchingHeuristics.h>

using namespace Glucose;
using namespace Candy;

//=================================================================================================
// Options:

namespace Candy {

//=================================================================================================
// Constructor/Destructor:

template<> Solver<RSILBranchingHeuristic3>::Solver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_) :
    // unsat certificate
    certificate(nullptr),
    // results
    model(), conflict(),
    // clauses
    clause_db(),
    // current assignment
    trail(),
    // propagate
    propagator(trail),
	// conflict analysis module
	conflict_analysis(trail, propagator),
	// branching heuristic
    branch(trail, conflict_analysis, std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_),
    // assumptions
    assumptions(),
    // restarts
    K(SolverOptions::opt_K), R(SolverOptions::opt_R), sumLBD(0),
    lbdQueue(SolverOptions::opt_size_lbd_queue), trailQueue(SolverOptions::opt_size_trail_queue),
    // reduce db heuristic control
    curRestart(0), nbclausesbeforereduce(SolverOptions::opt_first_reduce_db),
    incReduceDB(SolverOptions::opt_inc_reduce_db),
    // memory reorganization
    sort_watches(SolverOptions::opt_sort_watches),
    sort_variables(SolverOptions::opt_sort_variables),
    // simplify
    new_unary(false),
    // conflict state
    ok(true),
    // incremental mode
    incremental(false),
    // preprocessing
    preprocessing_enabled(true),
    freezes(),
    // inprocessing
    lastRestartWithInprocessing(0),
    inprocessingFrequency(SolverOptions::opt_inprocessing),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr),
    // sonification
    sonification(),
	controller()
{
controller.run();
}

template<> Solver<RSILBudgetBranchingHeuristic3>::Solver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_, uint64_t initialBudget_) :
    // unsat certificate
    certificate(nullptr),
    // results
    model(), conflict(),
    // clauses
    clause_db(),
    // current assignment
    trail(),
    // propagate
    propagator(trail),
	// conflict analysis module
	conflict_analysis(trail, propagator),
	// branching heuristic
    branch(trail, conflict_analysis, std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_, initialBudget_),
    // assumptions
    assumptions(),
    // restarts
    K(SolverOptions::opt_K), R(SolverOptions::opt_R), sumLBD(0),
    lbdQueue(SolverOptions::opt_size_lbd_queue), trailQueue(SolverOptions::opt_size_trail_queue),
    // reduce db heuristic control
    curRestart(0), nbclausesbeforereduce(SolverOptions::opt_first_reduce_db),
    incReduceDB(SolverOptions::opt_inc_reduce_db),
    // memory reorganization
    sort_watches(SolverOptions::opt_sort_watches),
    sort_variables(SolverOptions::opt_sort_variables),
    // simplify
    new_unary(false),
    // conflict state
    ok(true),
    // incremental mode
    incremental(false),
    // preprocessing
    preprocessing_enabled(true),
    freezes(),
    // inprocessing
    lastRestartWithInprocessing(0),
    inprocessingFrequency(SolverOptions::opt_inprocessing),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr),
    // sonification
    sonification(),
	controller()
{
controller.run();
}

template<> Solver<RSILVanishingBranchingHeuristic3>::Solver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_, uint64_t m_probHalfLife_) :
    // unsat certificate
    certificate(nullptr),
    // results
    model(), conflict(),
    // clauses
    clause_db(),
    // current assignment
    trail(),
    // propagate
    propagator(trail),
	// conflict analysis module
	conflict_analysis(trail, propagator),
	// branching heuristic
    branch(trail, conflict_analysis, std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_, m_probHalfLife_),
    // assumptions
    assumptions(),
    // restarts
    K(SolverOptions::opt_K), R(SolverOptions::opt_R), sumLBD(0),
    lbdQueue(SolverOptions::opt_size_lbd_queue), trailQueue(SolverOptions::opt_size_trail_queue),
    // reduce db heuristic control
    curRestart(0), nbclausesbeforereduce(SolverOptions::opt_first_reduce_db),
    incReduceDB(SolverOptions::opt_inc_reduce_db),
    // memory reorganization
    sort_watches(SolverOptions::opt_sort_watches),
    sort_variables(SolverOptions::opt_sort_variables),
    // simplify
    new_unary(false),
    // conflict state
    ok(true),
    // incremental mode
    incremental(false),
    // preprocessing
    preprocessing_enabled(true),
    freezes(),
    // inprocessing
    lastRestartWithInprocessing(0),
    inprocessingFrequency(SolverOptions::opt_inprocessing),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr),
    // sonification
    sonification(),
	controller()
{
controller.run();
}


//=================================================================================================
// Constructor/Destructor:

template<> Solver<RSILBranchingHeuristic2>::Solver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_) :
    // unsat certificate
    certificate(nullptr),
    // results
    model(), conflict(),
    // clauses
    clause_db(),
    // current assignment
    trail(),
    // propagate
    propagator(trail),
	// conflict analysis module
	conflict_analysis(trail, propagator),
	// branching heuristic
    branch(trail, conflict_analysis, std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_),
    // assumptions
    assumptions(),
    // restarts
    K(SolverOptions::opt_K), R(SolverOptions::opt_R), sumLBD(0),
    lbdQueue(SolverOptions::opt_size_lbd_queue), trailQueue(SolverOptions::opt_size_trail_queue),
    // reduce db heuristic control
    curRestart(0), nbclausesbeforereduce(SolverOptions::opt_first_reduce_db),
    incReduceDB(SolverOptions::opt_inc_reduce_db),
    // memory reorganization
    sort_watches(SolverOptions::opt_sort_watches),
    sort_variables(SolverOptions::opt_sort_variables),
    // simplify
    new_unary(false),
    // conflict state
    ok(true),
    // incremental mode
    incremental(false),
    // preprocessing
    preprocessing_enabled(true),
    freezes(),
    // inprocessing
    lastRestartWithInprocessing(0),
    inprocessingFrequency(SolverOptions::opt_inprocessing),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr),
    // sonification
    sonification(),
	controller()
{
controller.run();
}

template<> Solver<RSILBudgetBranchingHeuristic2>::Solver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_, uint64_t initialBudget_) :
    // unsat certificate
    certificate(nullptr),
    // results
    model(), conflict(),
    // clauses
    clause_db(),
    // current assignment
    trail(),
    // propagate
    propagator(trail),
	// conflict analysis module
	conflict_analysis(trail, propagator),
	// branching heuristic
    branch(trail, conflict_analysis, std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_, initialBudget_),
    // assumptions
    assumptions(),
    // restarts
    K(SolverOptions::opt_K), R(SolverOptions::opt_R), sumLBD(0),
    lbdQueue(SolverOptions::opt_size_lbd_queue), trailQueue(SolverOptions::opt_size_trail_queue),
    // reduce db heuristic control
    curRestart(0), nbclausesbeforereduce(SolverOptions::opt_first_reduce_db),
    incReduceDB(SolverOptions::opt_inc_reduce_db),
    // memory reorganization
    sort_watches(SolverOptions::opt_sort_watches),
    sort_variables(SolverOptions::opt_sort_variables),
    // simplify
    new_unary(false),
    // conflict state
    ok(true),
    // incremental mode
    incremental(false),
    // preprocessing
    preprocessing_enabled(true),
    freezes(),
    // inprocessing
    lastRestartWithInprocessing(0),
    inprocessingFrequency(SolverOptions::opt_inprocessing),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr),
    // sonification
    sonification(),
	controller()
{
controller.run();
}

template<> Solver<RSILVanishingBranchingHeuristic2>::Solver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_, uint64_t m_probHalfLife_) :
    // unsat certificate
    certificate(nullptr),
    // results
    model(), conflict(),
    // clauses
    clause_db(),
    // current assignment
    trail(),
    // propagate
    propagator(trail),
	// conflict analysis module
	conflict_analysis(trail, propagator),
	// branching heuristic
    branch(trail, conflict_analysis, std::move(conjectures), m_backbonesEnabled, rsar_filter_, filterOnlyBackbones_, m_probHalfLife_),
    // assumptions
    assumptions(),
    // restarts
    K(SolverOptions::opt_K), R(SolverOptions::opt_R), sumLBD(0),
    lbdQueue(SolverOptions::opt_size_lbd_queue), trailQueue(SolverOptions::opt_size_trail_queue),
    // reduce db heuristic control
    curRestart(0), nbclausesbeforereduce(SolverOptions::opt_first_reduce_db),
    incReduceDB(SolverOptions::opt_inc_reduce_db),
    // memory reorganization
    sort_watches(SolverOptions::opt_sort_watches),
    sort_variables(SolverOptions::opt_sort_variables),
    // simplify
    new_unary(false),
    // conflict state
    ok(true),
    // incremental mode
    incremental(false),
    // preprocessing
    preprocessing_enabled(true),
    freezes(),
    // inprocessing
    lastRestartWithInprocessing(0),
    inprocessingFrequency(SolverOptions::opt_inprocessing),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr),
    // sonification
    sonification(),
	controller()
{
controller.run();
}

}