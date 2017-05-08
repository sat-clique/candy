/***************************************************************************************[Solver.h]
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

#ifndef Glucose_Solver_h
#define Glucose_Solver_h

#include <vector>
#include <math.h>
#include <string>
#include <type_traits>

#include <candy/core/Statistics.h>
#include "candy/mtl/Heap.h"
#include "candy/utils/Options.h"
#include "candy/core/SolverTypes.h"
#include "candy/mtl/BoundedQueue.h"
#include "candy/core/Clause.h"
#include "candy/core/Certificate.h"
#include "candy/core/ClauseAllocator.h"
#include <candy/utils/CNFProblem.h>
#include "candy/utils/System.h"

#include "sonification/SolverSonification.h"

namespace Candy {

class DefaultPickBranchLit {
public:
    class Parameters {
        
    };
    
    DefaultPickBranchLit() noexcept {
        
    }
    
    explicit DefaultPickBranchLit(const Parameters& params) noexcept {
        (void)params;
    }
};

/**
 * \tparam PickBranchLitT   the PickBranchLit type used to choose a
 *   strategy for determining decision (ie. branching) literals.
 *   PickBranchLitT must satisfy the following conditions:
 *    - PickBranchLitT must be a class type.
 *    - PickBranchLitT::Parameters must be a class type.
 *    - PickBranchLitT must have a zero-argument constructor.
 *    - PickBranchLitT must have a constructor taking a single argument of type
 *        const Parameters& params.
 *    - PickBranchLitT must be move-assignable.
 *    - PickBranchLitT must be move-constructible.
 *    - There must be a specialization of Solver::pickBranchLit<PickBranchLitT>.
 */
template<class PickBranchLitT = DefaultPickBranchLit>
class Solver {
    static_assert(std::is_class<PickBranchLitT>::value, "PickBranchLitT must be a class");
    static_assert(std::is_class<typename PickBranchLitT::Parameters>::value,
                  "PickBranchLitT::Parameters must be a class");
    static_assert(std::is_constructible<PickBranchLitT>::value,
                  "PickBranchLitT must have a constructor without arguments");
    static_assert(std::is_move_assignable<PickBranchLitT>::value,
                  "PickBranchLitT must be move-assignable");
    static_assert(std::is_move_constructible<PickBranchLitT>::value,
                  "PickBranchLitT must be move-constructible");

    friend class SolverConfiguration;

public:
    using PickBranchLitType = PickBranchLitT;
    
    Solver();
    virtual ~Solver();
    
    /// Reinitializes the decision literal picking strategy using the given parameters.
    void initializePickBranchLit(const typename PickBranchLitT::Parameters& params);
    
    // Add a new variable with parameters specifying variable mode.
    virtual Var newVar(bool polarity = true, bool dvar = true);

    // Add a clause to the solver without making superflous internal copy. Will change ps
    virtual bool addClause_(vector<Lit>& ps);

    inline bool addClause(const vector<Lit>& ps) {
        add_tmp.clear();
        add_tmp.insert(add_tmp.end(), ps.begin(), ps.end());
        return addClause_(add_tmp);
    }
    inline bool addClause(std::initializer_list<Lit> lits) {
        add_tmp.clear();
        add_tmp.insert(add_tmp.end(), lits.begin(), lits.end());
        return addClause_(add_tmp);
    }

    void addClauses(CNFProblem dimacs);

    // use with care (written for solver tests only)
    Clause& getClause(size_t pos) {
        assert(pos < clauses.size());
        return *clauses[pos];
    }

    vector<Lit>& getConflict() {
        return conflict;
    }

    // Solving:
    bool simplify(); // Removes already satisfied clauses.

    virtual lbool solve(); // Main solve method (assumptions given in 'assumptions').

    inline lbool solve(std::initializer_list<Lit> assumps) {
        assumptions.clear();
        assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
        return solve();
    }

    inline lbool solve(const vector<Lit>& assumps) {
        assumptions.clear();
        assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
        return solve();
    }

    // true means solver is in a conflicting state
    inline bool isInConflictingState() const {
        return !ok;
    }

    // Declare if a variable should be eligible for selection in the decision heuristic.
    inline void setDecisionVar(Var v, bool b) {
        if (decision[v] != b) {
            decision[v] = b;
            if (b) {
                insertVarOrder(v);
                Statistics::getInstance().solverDecisionVariablesInc();
            } else {
                Statistics::getInstance().solverDecisionVariablesDec();
            }
        }
    }

    // The current value of a variable.
    inline lbool value(Var x) const {
        return assigns[x];
    }
    // The current value of a literal.
    inline lbool value(Lit p) const {
        return assigns[var(p)] ^ sign(p);
    }

    // The value of a variable in the last model. The last call to solve must have been satisfiable.
    lbool modelValue(Var x) const {
        return model[x];
    }
    // The value of a literal in the last model. The last call to solve must have been satisfiable.
    lbool modelValue(Lit p) const {
        return model[var(p)] ^ sign(p);
    }

    inline size_t nAssigns() const {
        return trail_size;
    }
    inline size_t nClauses() const {
        return clauses.size();
    }
    inline size_t nLearnts() const {
        return learnts.size() + learntsBin.size();
    }
    inline size_t nVars() const {
        return vardata.size();
    }

    // Incremental mode
    void setIncrementalMode();
    void initNbInitialVars(int nb);
    bool isIncremental();

    // Resource constraints:
    void setConfBudget(uint64_t x) {
        conflict_budget = nConflicts + x;
    }
    void setPropBudget(uint64_t x) {
        propagation_budget = nPropagations + x;
    }
    void setInterrupt(bool value) {
        asynch_interrupt = value;
    }
    void budgetOff() {
        conflict_budget = propagation_budget = 0;
    }

    //TODO: use std::function<int(void*)> as type here
    void setTermCallback(void* state, int (*termCallback)(void*)) {
        this->termCallbackState = state;
        this->termCallback = termCallback;
    }

    // Certified UNSAT (Thanks to Marijn Heule)
    Certificate certificate;

    // a few stats are used for heuristics control, keep them here
    uint64_t nConflicts, nPropagations, nLiterals;

    // Control verbosity
    uint16_t verbEveryConflicts;
    uint8_t verbosity;

    // Extra results: (read-only member variable)
    vector<lbool> model; // If problem is satisfiable, this vector contains the model (if any).
    vector<Lit> conflict; // If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.

protected:
	// Helper structures:
	struct VarData {
	    Clause* reason;
		uint_fast32_t level;
		VarData() :
		    reason(nullptr), level(0) {}
		VarData(Clause* _reason, uint_fast32_t _level) :
		    reason(_reason), level(_level) {}
	};

	struct Watcher {
	    Clause* cref;
		Lit blocker;
		Watcher() :
			cref(nullptr), blocker(lit_Undef) {}
		Watcher(Clause* cr, Lit p) :
			cref(cr), blocker(p) {}
		bool operator==(const Watcher& w) const {
			return cref == w.cref;
		}
		bool operator!=(const Watcher& w) const {
			return cref != w.cref;
		}
	};

	struct WatcherDeleted {
		WatcherDeleted() { }
		inline bool operator()(const Watcher& w) const {
			return w.cref->isDeleted() == 1;
		}
	};

	struct VarOrderLt {
		const vector<double>& activity;
		bool operator()(Var x, Var y) const {
			return activity[x] > activity[y];
		}
		VarOrderLt(const vector<double>& act) : activity(act) {}
	};

	struct reduceDB_lt {
	    reduceDB_lt() {
	    }

	    bool operator()(Clause* x, Clause* y) {
	//        // XMiniSat paper, alternate ordering
	//        const uint8_t reduceOnSizeSize = 12;
	//        uint32_t w1 = x->size() < reduceOnSizeSize : x->size() : x->size() + x->getLBD();
	//        uint32_t w2 = y->size() < reduceOnSizeSize : y->size() : y->size() + y->getLBD();
	//        return w1 > w2 || (w1 == w2 && x->activity() < y->activity());
	        return x->getLBD() > y->getLBD() || (x->getLBD() == y->getLBD() && x->activity() < y->activity());
	    }
	};

	ClauseAllocator allocator;

	// 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
    OccLists<Lit, Watcher, WatcherDeleted> watches;
    OccLists<Lit, Watcher, WatcherDeleted> watchesBin;

    vector<lbool> assigns; // The current assignments.
    vector<Lit> trail; // Assignment stack; stores all assigments made in the order they were made.
    uint32_t trail_size; // Current number of assignments (used to optimize propagate, through getting rid of capacity checking)
    uint32_t qhead; // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    vector<VarData> vardata; // Stores reason and level for each variable.
    vector<uint32_t> trail_lim; // Separator indices for different decision levels in 'trail'.
    vector<char> polarity; // The preferred polarity of each variable.
    vector<char> decision; // Declares if a variable is eligible for selection in the decision heuristic.
    vector<Lit> assumptions; // Current set of assumptions provided to solve by the user.

    // for activity based heuristics
    Glucose::Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable activity.
    vector<double> activity; // A heuristic measurement of the activity of a variable.
    double var_inc; // Amount to bump next variable with.
    double var_decay;
    double max_var_decay;
    double cla_inc; // Amount to bump next clause with.
    double clause_decay;

    // Clauses
    vector<Clause*> clauses; // List of problem clauses.
    vector<Clause*> learnts; // List of learnt clauses.
    vector<Clause*> learntsBin; // List of binary learnt clauses.

    // Constants For restarts
    double K;
    double R;
    float sumLBD = 0; // used to compute the global average of LBD. Restarts...
    // Bounded queues for restarts
    bqueue<uint32_t> lbdQueue, trailQueue;

    // used for reduceDB
    uint64_t curRestart;
    uint32_t nbclausesbeforereduce; // To know when it is time to reduce clause database
    uint16_t incReduceDB;
    uint16_t specialIncReduceDB;
    uint16_t lbLBDFrozenClause;

    // Constant for reducing clause
    uint16_t lbSizeMinimizingClause;
    uint16_t lbLBDMinimizingClause;

    uint8_t phase_saving; // Controls the level of phase saving (0=none, 1=limited, 2=full).

    // constants for memory reorganization
	uint8_t revamp;
    bool sort_watches;

    bool remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.
    bool unary_learnt; // Indicates whether a unary clause was learnt since the last restart

    bool ok; // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!

	vector<uint32_t> permDiff; // permDiff[var] contains the current conflict number... Used to count the number of  LBD
    uint32_t MYFLAG;

	// Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
	// used, exept 'seen' wich is used in several places.
	vector<char> seen;
	vector<Lit> analyze_toclear;
	vector<Lit> add_tmp;

	// Resource contraints and other interrupts
	uint32_t conflict_budget;    // 0 means no budget.
	uint32_t propagation_budget; // 0 means no budget.
    void* termCallbackState;
    int (*termCallback)(void* state);
    bool asynch_interrupt;

	// Variables added for incremental mode
    bool incremental; // Use incremental SAT Solver
	uint32_t nbVarsInitialFormula; // nb VAR in formula without assumptions (incremental SAT)

    // Sonification
    SolverSonification sonification;
    
    // Variables for decisions
    PickBranchLitT pickBranchLitData;

	// Main internal methods:
    inline Clause* reason(Var x) const {
        return vardata[x].reason;
    }

    inline int level(Var x) const {
        return vardata[x].level;
    }

    inline uint64_t abstractLevel(Var x) const {
        return 1ull << (level(x) & 63);
    }

    // Insert a variable in the decision order priority queue.
    inline void insertVarOrder(Var x) {
        if (!order_heap.inHeap(x) && decision[x])
            order_heap.insert(x);
    }

	// Begins a new decision level
    inline void newDecisionLevel() {
        trail_lim.push_back(trail_size);
    }

    // Returns TRUE if a clause is a reason for some implication in the current state.
    inline bool locked(Clause* cr) const {
        Clause& c = *cr;
        if (c.size() > 2) return value(c[0]) == l_True && reason(var(c[0])) == cr;
        return (value(c[0]) == l_True && reason(var(c[0])) == cr) || (value(c[1]) == l_True && reason(var(c[1])) == cr);
    }

    // Returns TRUE if a clause is satisfied in the current state.
    inline bool satisfied(const Clause& c) const {
        return std::any_of(c.begin(), c.end(), [this] (Lit lit) { return value(lit) == l_True; });
    }

    // Operations on clauses:
    void attachClause(Clause* cr); // Attach a clause to watcher lists.
    void detachClause(Clause* cr, bool strict = false); // Detach a clause to watcher lists.
    void removeClause(Clause* cr); // Detach and free a clause.
    void freeMarkedClauses(vector<Clause*>& list);

    template <typename Iterator>
    uint_fast16_t computeLBD(Iterator it, Iterator end);
    void minimisationWithBinaryResolution(vector<Lit> &out_learnt);

    // Test if fact 'p' contradicts current state, enqueue otherwise.
    // NOTE: enqueue does not set the ok flag! (only public methods do)
    inline bool enqueue(Lit p, Clause* from = nullptr) {
        return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true);
    }

    Lit pickBranchLit(); // Return the next decision variable.
    Lit defaultPickBranchLit(); // Return the next decision variable (default implementation).
	void uncheckedEnqueue(Lit p, Clause* from = nullptr); // Enqueue a literal. Assumes value of literal is undefined.
	Clause* propagate(); // Perform unit propagation. Returns possibly conflicting clause.
	void cancelUntil(int level); // Backtrack until a certain level.
	void analyze(Clause* confl, vector<Lit>& out_learnt, int& out_btlevel, uint_fast16_t &nblevels); // (bt = backtrack)
	void analyzeFinal(Lit p, vector<Lit>& out_conflict); // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
	bool litRedundant(Lit p, uint64_t abstract_levels); // (helper method for 'analyze()')
	lbool search(); // Search for a given number of conflicts.
	virtual void reduceDB(); // Reduce the set of learnt clauses.
	void rebuildOrderHeap();
    void revampClausePool(uint8_t upper);

	// Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
	inline void varDecayActivity() {
	    var_inc *= (1 / var_decay);
	}

	// Increase a variable with the current 'bump' value.
	inline void varBumpActivity(Var v) {
	    if (!isSelector(v)) {
	        varBumpActivity(v, var_inc);
	    }
	}

	inline void varBumpActivity(Var v, double inc) {
	    if ((activity[v] += inc) > 1e100) {
	        varRescaleActivity();
	    }
	    if (order_heap.inHeap(v)) {
	        order_heap.decrease(v); // update order-heap
	    }
	}

	void varRescaleActivity() {
        for (size_t i = 0; i < nVars(); i++) {
            activity[i] *= 1e-100;
        }
        var_inc *= 1e-100;
    }

	inline void claDecayActivity() {
	    cla_inc *= (1 / clause_decay);
	}

	inline void claBumpActivity(Clause& c) {
	    if ((c.activity() += cla_inc) > 1e20) {
	        claRescaleActivity();
	    }
	}

	void claRescaleActivity() {
	    for (auto container : { clauses, learnts, learntsBin }) {
	        for (Clause* clause : container) {
	            clause->activity() *= 1e-20;
	        }
	    }
        cla_inc *= 1e-20;
    }

	// Gives the current decisionlevel.
	inline uint32_t decisionLevel() const {
	    return trail_lim.size();
	}

    inline bool withinBudget() {
        return !asynch_interrupt && (termCallback == nullptr || 0 == termCallback(termCallbackState))
                && (conflict_budget == 0 || nConflicts < conflict_budget) && (propagation_budget == 0 || nPropagations < propagation_budget);
    }

	inline bool isSelector(Var v) {
		return (incremental && (uint32_t)v >= nbVarsInitialFormula);
	}
};

/**
 * \brief A readily forward-declarable Solver<>
 */
class DefaultSolver : public Solver<> {

};




//******************************************************************************
// Solver<PickBranchLitT> implementation
//******************************************************************************

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
    extern IntOption opt_spec_inc_reduce_db;
    extern IntOption opt_lb_lbd_frozen_clause;
    
    extern IntOption opt_lb_size_minimzing_clause;
    extern IntOption opt_lb_lbd_minimzing_clause;
    
    extern DoubleOption opt_var_decay;
    extern DoubleOption opt_max_var_decay;
    extern DoubleOption opt_clause_decay;
    extern IntOption opt_phase_saving;
    
    extern IntOption opt_sonification_delay;
    
    extern IntOption opt_revamp;
    extern BoolOption opt_sort_watches;
}

template<class PickBranchLitT>
Solver<PickBranchLitT>::Solver() :
    // unsat certificate
    certificate(nullptr, false),
    // stats for heuristic control
    nConflicts(0), nPropagations(0), nLiterals(0),
    // verbosity flags
    verbEveryConflicts(10000), verbosity(0),
    // results
    model(), conflict(),
    // clause allocator
    allocator(),
    // watchers
    watches(WatcherDeleted()), watchesBin(WatcherDeleted()),
    // current assignment
    assigns(), trail(), trail_size(0), qhead(0),
    vardata(), trail_lim(),
    polarity(), decision(),
    // assumptions
    assumptions(),
    // for activity based heuristics
    order_heap(VarOrderLt(activity)),
    activity(),
    var_inc(1), var_decay(SolverOptions::opt_var_decay), max_var_decay(SolverOptions::opt_max_var_decay),
    cla_inc(1), clause_decay(SolverOptions::opt_clause_decay),
    // clauses
    clauses(), learnts(), learntsBin(),
    // restarts
    K(SolverOptions::opt_K), R(SolverOptions::opt_R), sumLBD(0),
    lbdQueue(SolverOptions::opt_size_lbd_queue), trailQueue(SolverOptions::opt_size_trail_queue),
    // reduce db heuristic control
    curRestart(0), nbclausesbeforereduce(SolverOptions::opt_first_reduce_db),
    incReduceDB(SolverOptions::opt_inc_reduce_db), specialIncReduceDB(SolverOptions::opt_spec_inc_reduce_db),
    lbLBDFrozenClause(SolverOptions::opt_lb_lbd_frozen_clause),
    // clause reduction
    lbSizeMinimizingClause(SolverOptions::opt_lb_size_minimzing_clause),
    lbLBDMinimizingClause(SolverOptions::opt_lb_lbd_minimzing_clause),
    // phase saving
    phase_saving(SolverOptions::opt_phase_saving),
    // memory reorganization
    revamp(SolverOptions::opt_revamp), sort_watches(SolverOptions::opt_sort_watches),
    // simpdb
    remove_satisfied(true),
    unary_learnt(false),
    // conflict state
    ok(true),
    // lbd computation
    permDiff(), MYFLAG(0),
    // temporaries
    seen(), analyze_toclear(), add_tmp(),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // incremental related
    incremental(false), nbVarsInitialFormula(INT32_MAX),
    // sonification
    sonification(),
    pickBranchLitData() { }

template<class PickBranchLitT>
Solver<PickBranchLitT>::~Solver() {
    for (Clause* c : clauses) {
        allocator.deallocate(c);
    }
    for (Clause* c : learnts) {
        allocator.deallocate(c);
    }
    for (Clause* c : learntsBin) {
        allocator.deallocate(c);
    }
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::initializePickBranchLit(const typename PickBranchLitT::Parameters& params) {
    pickBranchLitData = PickBranchLitT(params);
}

/****************************************************************
 Set the incremental mode
 ****************************************************************/

// This function set the incremental mode to true.
// You can add special code for this mode here.
template<class PickBranchLitT>
void Solver<PickBranchLitT>::setIncrementalMode() {
    incremental = true;
}

// Number of variables without selectors
template<class PickBranchLitT>
void Solver<PickBranchLitT>::initNbInitialVars(int nb) {
    nbVarsInitialFormula = nb;
}

template<class PickBranchLitT>
bool Solver<PickBranchLitT>::isIncremental() {
    return incremental;
}

/***
 * Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
 * used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
 ***/
template<class PickBranchLitT>
Var Solver<PickBranchLitT>::newVar(bool sign, bool dvar) {
    int v = nVars();
    watches.init(mkLit(v, false));
    watches.init(mkLit(v, true));
    watchesBin.init(mkLit(v, false));
    watchesBin.init(mkLit(v, true));
    assigns.push_back(l_Undef);
    vardata.emplace_back();
    activity.push_back(0);
    seen.push_back(0);
    permDiff.push_back(0);
    polarity.push_back(sign);
    decision.push_back(0);
    trail.resize(v + 1);
    setDecisionVar(v, dvar);
    return v;
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::addClauses(CNFProblem dimacs) {
    vector<vector<Lit>*>& problem = dimacs.getProblem();
    if ((size_t)dimacs.nVars() > nVars()) {
        assigns.reserve(dimacs.nVars());
        vardata.reserve(dimacs.nVars());
        activity.reserve(dimacs.nVars());
        seen.reserve(dimacs.nVars());
        permDiff.reserve(dimacs.nVars());
        polarity.reserve(dimacs.nVars());
        decision.reserve(dimacs.nVars());
        trail.resize(dimacs.nVars());
        for (int i = nVars(); i < dimacs.nVars(); i++) {
            newVar();
        }
    }
    for (vector<Lit>* clause : problem) {
        addClause(*clause);
    }
}

template<class PickBranchLitT>
bool Solver<PickBranchLitT>::addClause_(vector<Lit>& ps) {
    assert(decisionLevel() == 0);
    if (!ok)
        return false;
    
    std::sort(ps.begin(), ps.end());
    
    vector<Lit> oc;
    if (certificate.isActive()) {
        auto pos = find_if(ps.begin(), ps.end(), [this](Lit lit) { return value(lit) == l_True || value(lit) == l_False; });
        if (pos != ps.end()) oc.insert(oc.end(), ps.begin(), ps.end());
    }
    
    Lit p;
    uint_fast16_t i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++) {
        if (value(ps[i]) == l_True || ps[i] == ~p) {
            return true;
        }
        else if (value(ps[i]) != l_False && ps[i] != p) {
            ps[j++] = p = ps[i];
        }
    }
    ps.resize(j);
    
    if (oc.size() > 0) {
        certificate.learnt(ps);
        certificate.removed(oc);
    }
    
    if (ps.size() == 0) {
        return ok = false;
    } else if (ps.size() == 1) {
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == nullptr);
    } else {
        Clause* cr = new (allocator.allocate(ps.size())) Clause(ps);
        clauses.push_back(cr);
        attachClause(cr);
    }
    
    return true;
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::attachClause(Clause* cr) {
    assert(cr->size() > 1);
    
    nLiterals += cr->size();
    
    if (cr->size() == 2) {
        watchesBin[~cr->first()].emplace_back(cr, cr->second());
        watchesBin[~cr->second()].emplace_back(cr, cr->first());
    } else {
        watches[~cr->first()].emplace_back(cr, cr->second());
        watches[~cr->second()].emplace_back(cr, cr->first());
    }
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::detachClause(Clause* cr, bool strict) {
    assert(cr->size() > 1);
    
    nLiterals -= cr->size();
    
    if (cr->size() == 2) {
        if (strict) {
            vector<Watcher>& list0 = watchesBin[~cr->first()];
            vector<Watcher>& list1 = watchesBin[~cr->second()];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [cr](Watcher w){ return w.cref == cr; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [cr](Watcher w){ return w.cref == cr; }), list1.end());
        } else {
            watchesBin.smudge(~cr->first());
            watchesBin.smudge(~cr->second());
        }
    } else {
        if (strict) {
            vector<Watcher>& list0 = watches[~cr->first()];
            vector<Watcher>& list1 = watches[~cr->second()];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [cr](Watcher w){ return w.cref == cr; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [cr](Watcher w){ return w.cref == cr; }), list1.end());
        } else {
            watches.smudge(~cr->first());
            watches.smudge(~cr->second());
        }
    }
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::removeClause(Clause* cr) {
    certificate.removed(cr);
    detachClause(cr);
    // Don't leave pointers to free'd memory!
    if (locked(cr)) {
        vardata[var(cr->first())].reason = nullptr;
    }
    cr->setDeleted();
}

/************************************************************
 * Compute LBD functions
 *************************************************************/
template<class PickBranchLitT>
template <typename Iterator>
inline uint_fast16_t Solver<PickBranchLitT>::computeLBD(Iterator it, Iterator end) {
    uint_fast16_t nblevels = 0;
    MYFLAG++;
    
    if (incremental) { // INCREMENTAL MODE
        for (; it != end; it++) {
            Lit lit = *it;
            if (isSelector(var(lit))) {
                continue;
            }
            int l = level(var(lit));
            if (permDiff[l] != MYFLAG) {
                permDiff[l] = MYFLAG;
                nblevels++;
            }
        }
    } else { // DEFAULT MODE
        for (; it != end; it++) {
            Lit lit = *it;
            int l = level(var(lit));
            if (permDiff[l] != MYFLAG) {
                permDiff[l] = MYFLAG;
                nblevels++;
            }
        }
    }
    
    return nblevels;
}

/******************************************************************
 * Minimisation with binary resolution
 ******************************************************************/
template <class PickBranchLitT>
void Solver<PickBranchLitT>::minimisationWithBinaryResolution(vector<Lit>& out_learnt) {
    uint_fast16_t lbd = computeLBD(out_learnt.begin(), out_learnt.end());
    
    if (lbd <= lbLBDMinimizingClause) {
        MYFLAG++;
        
        for (auto it = out_learnt.begin() + 1; it != out_learnt.end(); it++) {
            permDiff[var(*it)] = MYFLAG;
        }
        
        uint_fast16_t nb = 0;
        for (Watcher w : watchesBin[~out_learnt[0]]) {
            if (permDiff[var(w.blocker)] == MYFLAG && value(w.blocker) == l_True) {
                nb++;
                permDiff[var(w.blocker)] = MYFLAG - 1;
            }
        }
        if (nb > 0) {
            Statistics::getInstance().solverReducedClausesInc(nb);
            for (uint_fast16_t i = 1, l = out_learnt.size()-1; i < out_learnt.size() - nb; i++) {
                if (permDiff[var(out_learnt[i])] != MYFLAG) {
                    std::swap(out_learnt[i], out_learnt[l]);
                    l--;
                    i--;
                }
            }
            out_learnt.resize(out_learnt.size() - nb);
        }
    }
}

// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
template <class PickBranchLitT>
void Solver<PickBranchLitT>::cancelUntil(int level) {
    if ((int)decisionLevel() > level) {
        for (int c = trail_size - 1; c >= (int)trail_lim[level]; c--) {
            Var x = var(trail[c]);
            assigns[x] = l_Undef;
            if (phase_saving > 1 || ((phase_saving == 1) && c > (int)trail_lim.back())) {
                polarity[x] = sign(trail[c]);
            }
            insertVarOrder(x);
        }
        qhead = trail_lim[level];
        trail_size = trail_lim[level];
        trail_lim.resize(level);
    }
}

/**************************************************************************************************
 *
 *  analyze : (confl : Clause*) (out_learnt : vector<Lit>&) (out_btlevel : int&)  ->  [void]
 *
 *  Description:
 *    Analyze conflict and produce a reason clause.
 *
 *    Pre-conditions:
 *      - 'out_learnt' is assumed to be cleared.
 *      - Current decision level must be greater than root level.
 *
 *    Post-conditions:
 *      - 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
 *      - If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
 *        rest of literals. There may be others from the same level though.
 *
 ***************************************************************************************************/
template <class PickBranchLitT>
void Solver<PickBranchLitT>::analyze(Clause* confl, vector<Lit>& out_learnt, int& out_btlevel, uint_fast16_t &lbd) {
    int pathC = 0;
    Lit asslit = lit_Undef;
    vector<Lit> selectors;
    vector<Lit> lastDecisionLevel; // UPDATEVARACTIVITY trick (see competition'09 companion paper)
    
    // Generate conflict clause:
    out_learnt.push_back(lit_Undef); // (leave room for the asserting literal)
    int index = trail_size - 1;
    do {
        assert(confl != nullptr); // (otherwise should be UIP)
        Clause& c = *confl;
        
        // Special case for binary clauses: The first one has to be SAT
        if (asslit != lit_Undef && c.size() == 2 && value(c[0]) == l_False) {
            assert(value(c[1]) == l_True);
            c.swap(0, 1);
        }
        
        claBumpActivity(c);
        
        // DYNAMIC NBLEVEL trick (see competition'09 companion paper)
        if (c.isLearnt() && c.getLBD() > 2) {
            uint_fast16_t nblevels = computeLBD(c.begin(), c.end());
            if (nblevels + 1 < c.getLBD()) { // improve the LBD
                if (c.getLBD() <= lbLBDFrozenClause) {
                    c.setFrozen(true);
                }
                // seems to be interesting : keep it for the next round
                c.setLBD(nblevels); // Update it
            }
        }
        
        for (uint_fast16_t j = (asslit == lit_Undef) ? 0 : 1; j < c.size(); j++) {
            Lit lit = c[j];
            
            if (!seen[var(lit)] && level(var(lit)) != 0) {
                varBumpActivity(var(lit));
                seen[var(lit)] = 1;
                if (level(var(lit)) >= (int)decisionLevel()) {
                    pathC++;
                    // UPDATEVARACTIVITY trick (see competition'09 companion paper)
                    if ((reason(var(lit)) != nullptr) && reason(var(lit))->isLearnt()) {
                        lastDecisionLevel.push_back(lit);
                    }
                } else {
                    if (isSelector(var(lit))) {
                        assert(value(lit) == l_False);
                        selectors.push_back(lit);
                    } else {
                        out_learnt.push_back(lit);
                    }
                }
            }
        }
        
        // Select next clause to look at:
        while (!seen[var(trail[index])]) {
            index--;
        }
        
        asslit = trail[index];
        confl = reason(var(asslit));
        seen[var(asslit)] = 0;
        pathC--;
    } while (pathC > 0);
    
    out_learnt[0] = ~asslit;
    
    // Simplify conflict clause:
    out_learnt.insert(out_learnt.end(), selectors.begin(), selectors.end());
    Statistics::getInstance().solverMaxLiteralsInc(out_learnt.size());
    
    analyze_toclear.clear();
    analyze_toclear.insert(analyze_toclear.end(), out_learnt.begin(), out_learnt.end());
    
    // minimize clause
    uint64_t abstract_level = 0;
    for (uint_fast16_t i = 1; i < out_learnt.size(); i++) {
        abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)
    }
    auto end = remove_if(out_learnt.begin()+1, out_learnt.end(),
                         [this, abstract_level] (Lit lit) {
                             return reason(var(lit)) != nullptr && litRedundant(lit, abstract_level);
                         });
    out_learnt.erase(end, out_learnt.end());
    
    assert(out_learnt[0] == ~asslit);
    
    Statistics::getInstance().solverTotLiteralsInc(out_learnt.size());
    
    /* Minimisation with binary clauses of the asserting clause */
    if (!incremental && out_learnt.size() <= lbSizeMinimizingClause) {
        minimisationWithBinaryResolution(out_learnt);
    }
    
    // Find correct backtrack level:
    if (out_learnt.size() == 1) {
        out_btlevel = 0;
    }
    else {
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (uint_fast16_t i = 2; i < out_learnt.size(); i++)
            if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
                max_i = i;
        // Swap-in this literal at index 1:
        out_btlevel = level(var(out_learnt[max_i]));
        std::swap(out_learnt[max_i], out_learnt[1]);
    }
    
    // Compute LBD
    lbd = computeLBD(out_learnt.begin(), out_learnt.end() - selectors.size());
    
    // UPDATEVARACTIVITY trick (see competition'09 companion paper)
    if (lastDecisionLevel.size() > 0) {
        for (Lit lit : lastDecisionLevel) {
            if (reason(var(lit))->getLBD() < lbd)
                varBumpActivity(var(lit));
        }
        lastDecisionLevel.clear();
    }

    // clear seen[]
    for_each(analyze_toclear.begin(), analyze_toclear.end(), [this] (Lit lit) { seen[var(lit)] = 0; });
}

// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
template <class PickBranchLitT>
bool Solver<PickBranchLitT>::litRedundant(Lit p, uint64_t abstract_levels) {
    static vector<Lit> analyze_stack;
    analyze_stack.clear();
    analyze_stack.push_back(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0) {
        assert(reason(var(analyze_stack.back())) != nullptr);
        Clause& c = *reason(var(analyze_stack.back()));
        analyze_stack.pop_back();
        if (c.size() == 2 && value(c[0]) == l_False) {
            assert(value(c[1]) == l_True);
            c.swap(0, 1);
        }
        
        for (Lit p : c) {
            if (!seen[var(p)] && level(var(p)) > 0) {
                if (reason(var(p)) != nullptr && (abstractLevel(var(p)) & abstract_levels) != 0) {
                    seen[var(p)] = 1;
                    analyze_stack.push_back(p);
                    analyze_toclear.push_back(p);
                } else {
                    for (unsigned int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.resize(top);
                    return false;
                }
            }
        }
    }
    
    return true;
}

/**************************************************************************************************
 *
 *  analyzeFinal : (p : Lit)  ->  [void]
 *
 *  Description:
 *    Specialized analysis procedure to express the final conflict in terms of assumptions.
 *    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
 *    stores the result in 'out_conflict'.
 |*************************************************************************************************/
template <class PickBranchLitT>
void Solver<PickBranchLitT>::analyzeFinal(Lit p, vector<Lit>& out_conflict) {
    out_conflict.clear();
    out_conflict.push_back(p);
    
    if (decisionLevel() == 0)
        return;
    
    seen[var(p)] = 1;
    
    for (int i = trail_size - 1; i >= (int)trail_lim[0]; i--) {
        Var x = var(trail[i]);
        if (seen[x]) {
            if (reason(x) == nullptr) {
                assert(level(x) > 0);
                out_conflict.push_back(~trail[i]);
            } else {
                Clause& c = *reason(x);
                //                for (int j = 1; j < c.size(); j++) Minisat (glucose 2.0) loop
                // Bug in case of assumptions due to special data structures for Binary.
                // Many thanks to Sam Bayless (sbayless@cs.ubc.ca) for discover this bug.
                for (uint_fast16_t j = ((c.size() == 2) ? 0 : 1); j < c.size(); j++)
                    if (level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
            }
            
            seen[x] = 0;
        }
    }
    
    seen[var(p)] = 0;
}

template <class PickBranchLitT>
void Solver<PickBranchLitT>::uncheckedEnqueue(Lit p, Clause* from) {
    assert(value(p) == l_Undef);
    assigns[var(p)] = lbool(!sign(p));
    vardata[var(p)] = VarData(from, decisionLevel());
    trail[trail_size++] = p;
}

/**************************************************************************************************
 *
 *  propagate : [void]  ->  [Clause*]
 *
 *  Description:
 *    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
 *    otherwise CRef_Undef.
 *
 *    Post-conditions:
 *      * the propagation queue is empty, even if there was a conflict.
 **************************************************************************************************/
template <class PickBranchLitT>
Clause* Solver<PickBranchLitT>::propagate() {
    Clause* confl = nullptr;
    uint32_t old_qhead = qhead;
    
    watches.cleanAll();
    watchesBin.cleanAll();
    
    while (qhead < trail_size) {
        Lit p = trail[qhead++]; // 'p' is enqueued fact to propagate.
        
        // Propagate binary clauses
        vector<Watcher>& wbin = watchesBin[p];
        for (auto watcher : wbin) {
            if (value(watcher.blocker) == l_False) {
                return watcher.cref;
            }
            if (value(watcher.blocker) == l_Undef) {
                uncheckedEnqueue(watcher.blocker, watcher.cref);
            }
        }
        
        // Propagate other 2-watched clauses
        std::vector<Watcher>& ws = watches[p];
        auto keep = ws.begin();
        for (auto watcher = ws.begin(); watcher != ws.end();) {
            // Try to avoid inspecting the clause:
            Lit blocker = watcher->blocker;
            if (value(blocker) == l_True) {
                *keep++ = *watcher++;
                continue;
            }
            
            // Make sure the false literal is data[1]:
            Clause* cr = watcher->cref;
            if (cr->first() == ~p) {
                cr->swap(0, 1);
            }
            
            watcher++;
            // If 0th watch is true, then clause is already satisfied.
            Watcher w = Watcher(cr, cr->first());
            if (cr->first() != blocker && value(cr->first()) == l_True) {
                *keep++ = w;
                continue;
            }
            
            if (incremental) { // INCREMENTAL MODE
                Clause& c = *cr;
                int watchesUpdateLiteralIndex = -1;
                for (uint_fast16_t k = 2; k < c.size(); k++) {
                    if (value(c[k]) != l_False) {
                        watchesUpdateLiteralIndex = k;
                        if (decisionLevel() > assumptions.size() || value(c[k]) == l_True || !isSelector(var(c[k]))) {
                            break;
                        }
                    }
                }
                
                if (watchesUpdateLiteralIndex != -1) {
                    c.swap(1, watchesUpdateLiteralIndex);
                    watches[~c[1]].push_back(w);
                    goto NextClause;
                }
            }
            else { // DEFAULT MODE (NOT INCREMENTAL)
                Clause& c = *cr;
                for (uint_fast16_t k = 2; k < c.size(); k++) {
                    if (value(c[k]) != l_False) {
                        c.swap(1, k);
                        watches[~c[1]].push_back(w);
                        goto NextClause;
                    }
                }
            }
            
            // Did not find watch -- clause is unit under assignment:
            *keep++ = w;
            if (value(cr->first()) == l_False) {
                confl = cr;
                qhead = trail_size;
                while (watcher != ws.end()) { // Copy the remaining watches
                    *keep++ = *watcher++;
                }
            }
            else {
                uncheckedEnqueue(cr->first(), cr);
            }
            
        NextClause: ;
        }
        ws.erase(keep, ws.end());
    }
    
    nPropagations += qhead - old_qhead;

    return confl;
}

/**************************************************************************************************
 *
 *  reduceDB : ()  ->  [void]
 *
 *  Description:
 *    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
 *    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
 **************************************************************************************************/
template <class PickBranchLitT>
void Solver<PickBranchLitT>::reduceDB() {
    Statistics::getInstance().solverReduceDBInc();
    std::sort(learnts.begin(), learnts.end(), reduceDB_lt());
    
    size_t index = (learnts.size() + learntsBin.size()) / 2;
    if (index >= learnts.size() || learnts[index]->getLBD() <= 3) {
        // We have a lot of "good" clauses, it is difficult to compare them. Keep more !
        if (specialIncReduceDB == 0) {
            return;
        } else {
            nbclausesbeforereduce += specialIncReduceDB;
        }
    }
    
    // Delete clauses from the first half which are not locked. (Binary clauses are kept separately and are not touched here)
    // Keep clauses which seem to be useful (i.e. their lbd was reduce during this sequence => frozen)
    size_t limit = std::min(learnts.size(), index);
    for (size_t i = 0; i < limit; i++) {
        Clause* c = learnts[i];
        if (c->getLBD() <= 2) break; // small lbds come last in sequence (see ordering by reduceDB_lt())
        if (!c->isFrozen() && (value(c->first()) != l_True || reason(var(c->first())) != c)) {//&& !locked(c)) {
            removeClause(c);
        }
    }
    watches.cleanAll();
    freeMarkedClauses(learnts);
    for (Clause* c : learnts) { // "unfreeze" remaining clauses
        c->setFrozen(false);
    }
}

/**
 * Make sure all references are cleared before space is handed back to ClauseAllocator
 */
template <class PickBranchLitT>
void Solver<PickBranchLitT>::freeMarkedClauses(vector<Clause*>& list) {
    auto new_end = std::remove_if(list.begin(), list.end(),
                                  [this](Clause* c) {
                                      if (c->isDeleted()) {
                                          allocator.deallocate(c);
                                          return true;
                                      } else {
                                          return false;
                                      }
                                  });
    Statistics::getInstance().solverRemovedClausesInc(std::distance(new_end, list.end()));
    list.erase(new_end, list.end());
}

template <class PickBranchLitT>
void Solver<PickBranchLitT>::rebuildOrderHeap() {
    vector<Var> vs;
    for (size_t v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef)
            vs.push_back(v);
    order_heap.build(vs);
}

template <class PickBranchLitT>
void Solver<PickBranchLitT>::revampClausePool(uint_fast8_t upper) {
    assert(upper <= REVAMPABLE_PAGES_MAX_SIZE);
    
    Statistics::getInstance().runtimeStart("Runtime Revamp");
    
    size_t old_clauses_size = clauses.size();
    size_t old_learnts_size = learnts.size();
    
    // save trail of unary propagations
    vector<Lit> props(trail.begin(), trail.begin() + trail_size);
    for (Lit p : props) {
        assigns[var(p)] = l_Undef;
        vardata[var(p)] = VarData(nullptr, 0);
    }
    trail_size = 0;

    // detach them
    for (Clause* c : clauses) {
        if (c->size() > 2 && c->size() <= upper) {
            assert(!locked(c)); assert(!c->isDeleted());
            detachClause(c, true);
        }
    }
    
    for (Clause* c : learnts) {
        assert(c->size() > 2); // binaries are in learntsBin
        if (c->size() <= upper) {
            assert(!locked(c)); assert(!c->isDeleted());
            detachClause(c, true);
        }
    }
    
    // remove them
    clauses.erase(std::remove_if(clauses.begin(), clauses.end(), [this,upper](Clause* c) {
        return (c->size() > 2 && c->size() <= upper); } ), clauses.end());
    
    learnts.erase(std::remove_if(learnts.begin(), learnts.end(), [this,upper](Clause* c) {
        return (c->size() > 2 && c->size() <= upper); } ), learnts.end());
    
    // revamp  and reattach them
    for (uint_fast8_t k = 3; k <= upper; k++) {
        vector<Clause*> revamped = allocator.revampPages(k);
        for (Clause* clause : revamped) {
            attachClause(clause);
            if (clause->isLearnt()) {
                learnts.push_back(clause);
            } else {
                clauses.push_back(clause);
            }
        }
    }
    
    // restore trail of unary propagation
    for (Lit p : props) {
        if (assigns[var(p)] == l_Undef) {
            uncheckedEnqueue(p);
            propagate();
        }
    }

    Statistics::getInstance().runtimeStop("Runtime Revamp");
    
    assert(old_learnts_size == learnts.size());
    assert(old_clauses_size == clauses.size());
    (void)(old_learnts_size);
    (void)(old_clauses_size);
}

/**************************************************************************************************
 *
 *  simplify : [void]  ->  [bool]
 *
 *  Description:
 *    Simplify the clause database according to the current top-level assignment. Currently, the only
 *    thing done here is the removal of satisfied clauses, but more things can be put here.
 **************************************************************************************************/
template <class PickBranchLitT>
bool Solver<PickBranchLitT>::simplify() {
    assert(decisionLevel() == 0);
    
    if (!ok || propagate() != nullptr) {
        return ok = false;
    }
    
    Statistics::getInstance().runtimeStart("Runtime Simplify");

    // Remove satisfied clauses:
    std::for_each(learnts.begin(), learnts.end(), [this] (Clause* c) { if (satisfied(*c)) removeClause(c); } );
    std::for_each(learntsBin.begin(), learntsBin.end(), [this] (Clause* c) { if (satisfied(*c)) removeClause(c); } );
    if (remove_satisfied) { // Can be turned off.
        std::for_each(clauses.begin(), clauses.end(), [this] (Clause* c) { if (satisfied(*c)) removeClause(c); });
    }
    watches.cleanAll();
    watchesBin.cleanAll();
    freeMarkedClauses(learnts);
    freeMarkedClauses(learntsBin);
    freeMarkedClauses(clauses);
    
    rebuildOrderHeap();
    
    Statistics::getInstance().runtimeStop("Runtime Simplify");

    return true;
}

/**************************************************************************************************
 *
 *  search : (nof*conflicts : int) (params : const SearchParams&)  ->  [lbool]
 *
 *  Description:
 *    Search for a model the specified number of conflicts.
 *    NOTE! Use negative value for 'nof*conflicts' indicate infinity.
 *
 *  Output:
 *    'l*True' if a partial assigment that is consistent with respect to the clauseset is found. If
 *    all variables are decision variables, this means that the clause set is satisfiable. 'l*False'
 *    if the clause set is unsatisfiable. 'l*Undef' if the bound on number of conflicts is reached.
 **************************************************************************************************/
template <class PickBranchLitT>
lbool Solver<PickBranchLitT>::search() {
    assert(ok);
    
    int backtrack_level;
    vector<Lit> learnt_clause;
    uint_fast16_t nblevels;
    bool blocked = false;
    bool reduced = false;
    Statistics::getInstance().solverRestartInc();
    sonification.restart();
    for (;;) {
        sonification.decisionLevel(decisionLevel(), SolverOptions::opt_sonification_delay);
        
        Clause* confl = propagate();
        
        sonification.assignmentLevel(nAssigns());
        
        if (confl != nullptr) { // CONFLICT
            sonification.conflictLevel(decisionLevel());
            
            ++nConflicts;
            
            if (nConflicts % 5000 == 0 && var_decay < max_var_decay) {
                var_decay += 0.01;
            }
            
            if (verbosity >= 1 && nConflicts % verbEveryConflicts == 0) {
                Statistics::getInstance().printIntermediateStats((int) (trail_lim.size() == 0 ? trail_size : trail_lim[0]), nClauses(), nLearnts(), nConflicts, nLiterals);
            }
            if (decisionLevel() == 0) {
                return l_False;
            }
            
            trailQueue.push(trail_size);
            
            // BLOCK RESTART (CP 2012 paper)
            if (nConflicts > 10000 && lbdQueue.isvalid() && trail_size > R * trailQueue.getavg()) {
                lbdQueue.fastclear();
                Statistics::getInstance().solverStopsRestartsInc();
                if (!blocked) {
                    Statistics::getInstance().solverLastBlockAtRestartSave();
                    Statistics::getInstance().solverStopsRestartsSameInc();
                    blocked = true;
                }
            }
            
            learnt_clause.clear();
            
            analyze(confl, learnt_clause, backtrack_level, nblevels);
            
            sonification.learntSize(learnt_clause.size());
            
            lbdQueue.push(nblevels);
            sumLBD += nblevels;
            
            cancelUntil(backtrack_level);
            
            certificate.learnt(learnt_clause);
            
            if (learnt_clause.size() == 1) {
                uncheckedEnqueue(learnt_clause[0]);
                unary_learnt = true;
                Statistics::getInstance().solverUnariesInc();
            }
            else {
                Clause* cr = new ((allocator.allocate(learnt_clause.size()))) Clause(learnt_clause, nblevels);
                if (nblevels <= 2) {
                    Statistics::getInstance().solverLBD2Inc();
                }
                if (cr->size() == 2) {
                    learntsBin.push_back(cr);
                    Statistics::getInstance().solverBinariesInc();
                }
                else {
                    learnts.push_back(cr);
                }
                claBumpActivity(*cr);
                attachClause(cr);
                uncheckedEnqueue(cr->first(), cr);
            }
            varDecayActivity();
            claDecayActivity();
        }
        else {
            // Our dynamic restart, see the SAT09 competition compagnion paper
            if ((lbdQueue.isvalid() && ((lbdQueue.getavg() * K) > (sumLBD / nConflicts)))) {
                lbdQueue.fastclear();
                
                if (incremental) { // DO NOT BACKTRACK UNTIL 0.. USELESS
                    size_t bt = (decisionLevel() < assumptions.size()) ? decisionLevel() : assumptions.size();
                    cancelUntil(bt);
                } else {
                    cancelUntil(0);
                }
                
                if (unary_learnt) {
                    cancelUntil(0);
                    if (!simplify()) {
                        return l_False;
                    }
                    unary_learnt = false;
                }
                
                // every restart after reduce-db
                if (reduced) {
                    if (revamp > 2) {
                        cancelUntil(0);
                        revampClausePool(revamp);
                    }
                    
                    if (sort_watches) {
                        Statistics::getInstance().runtimeStart("Runtime Sort Watches");
                        for (size_t v = 0; v < nVars(); v++) {
                            for (Lit l : { mkLit(v, false), mkLit(v, true) }) {
                                sort(watches[l].begin(), watches[l].end(), [](Watcher w1, Watcher w2) {
                                    Clause& c1 = *w1.cref;
                                    Clause& c2 = *w2.cref;
                                    return c1.size() < c2.size() || (c1.size() == c2.size() && c1.activity() > c2.activity());
                                });
                            }
                        }
                        Statistics::getInstance().runtimeStop("Runtime Sort Watches");
                    }
                    
                    reduced = false;
                }
                
                return l_Undef;
            }
            
            // Perform clause database reduction !
            if (nConflicts >= (curRestart * nbclausesbeforereduce)) {
                if (learnts.size() > 0) {
                    curRestart = (nConflicts / nbclausesbeforereduce) + 1;
                    reduceDB();
                    reduced = true;
                    nbclausesbeforereduce += incReduceDB;
                }
            }
            
            Lit next = lit_Undef;
            while (decisionLevel() < assumptions.size()) {
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if (value(p) == l_True) {
                    // Dummy decision level:
                    newDecisionLevel();
                } else if (value(p) == l_False) {
                    analyzeFinal(~p, conflict);
                    return l_False;
                } else {
                    next = p;
                    break;
                }
            }
            
            if (next == lit_Undef) {
                // New variable decision:
                Statistics::getInstance().solverDecisionsInc();
                next = pickBranchLit();
                if (next == lit_Undef) {
                    // Model found:
                    return l_True;
                }
            }
            
            // Increase decision level and enqueue 'next'
            newDecisionLevel();
            uncheckedEnqueue(next);
        }
    }
    return l_Undef; // not reached
}

// NOTE: assumptions passed in member-variable 'assumptions'.
// Parameters are useless in core but useful for SimpSolver....
template <class PickBranchLitT>
lbool Solver<PickBranchLitT>::solve() {
    Statistics::getInstance().runtimeStart(RT_SOLVER);
    
    if (incremental && certificate.isActive()) {
        printf("Can not use incremental and certified unsat in the same time\n");
        exit(-1);
    }
    
    model.clear();
    conflict.clear();
    if (!ok) {
        Statistics::getInstance().runtimeStop(RT_SOLVER);
        return l_False;
    }
    
    double curTime = Glucose::cpuTime();
    
    sonification.start(nVars(), nClauses());
    
    if (!incremental && verbosity >= 1) {
        printf("c =====================================[ MAGIC CONSTANTS ]======================================\n");
        printf("c | Constants are supposed to work well together :-)                                           |\n");
        printf("c | however, if you find better choices, please let us known...                                |\n");
        printf("c |--------------------------------------------------------------------------------------------|\n");
        printf("c |                                |                                |                          |\n");
        printf("c | - Restarts:                    | - Reduce Clause DB:            | - Minimize Asserting:    |\n");
        printf("c |   * LBD Queue    : %6d      |   * First     : %6d         |    * size < %3d          |\n", lbdQueue.maxSize(), nbclausesbeforereduce, lbSizeMinimizingClause);
        printf("c |   * Trail  Queue : %6d      |   * Inc       : %6d         |    * lbd  < %3d          |\n", trailQueue.maxSize(), incReduceDB,
               lbLBDMinimizingClause);
        printf("c |   * K            : %6.2f      |   * Special   : %6d         |                          |\n", K, specialIncReduceDB);
        printf("c |   * R            : %6.2f      |   * Protected :  (lbd)< %2d     |                          |\n", R, lbLBDFrozenClause);
        printf("c |                                |                                |                          |\n");
        printf("c =========================[ Search Statistics (every %6d conflicts) ]=======================\n", verbEveryConflicts);
        printf("c |                                                                                            |\n");
        
        printf("c |          RESTARTS           |          ORIGINAL         |              LEARNT              |\n");
        printf("c |       NB   Blocked  Avg Cfc |    Vars  Clauses Literals |   Red   Learnts    LBD2  Removed |\n");
        printf("c ==============================================================================================\n");
    }
    
    // Search:
    lbool status = l_Undef;
    while (status == l_Undef && withinBudget()) {
        status = search();
    }
    
    double finalTime = Glucose::cpuTime();
    
    if (status == l_False) {
        Statistics::getInstance().incNBUnsatCalls();
        Statistics::getInstance().incTotalTime4Unsat(finalTime - curTime);
        
        certificate.proof();
        
        if (conflict.size() == 0) {
            ok = false;
        }

        sonification.stop(1);
    }
    else if (status == l_True) {
        Statistics::getInstance().incNBSatCalls();
        Statistics::getInstance().incTotalTime4Sat(finalTime - curTime);
        
        model.clear();
        model.insert(model.end(), assigns.begin(), assigns.end());

        sonification.stop(0);
    }
    else {
        sonification.stop(-1);
    }
    
    cancelUntil(0);
    
    Statistics::getInstance().runtimeStop(RT_SOLVER);
    return status;
}

template <class PickBranchLitT>
__attribute__((always_inline))
inline Lit Solver<PickBranchLitT>::defaultPickBranchLit() {
    Var next = var_Undef;
    
    // Activity based decision:
    while (next == var_Undef || value(next) != l_Undef || !decision[next]) {
        if (order_heap.empty()) {
            next = var_Undef;
            break;
        } else {
            next = order_heap.removeMin();
        }
    }
    
    return next == var_Undef ? lit_Undef : mkLit(next, polarity[next]);
}

}

#endif
