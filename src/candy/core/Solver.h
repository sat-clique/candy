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

#include <candy/core/Statistics.h>
#include "candy/mtl/Heap.h"
#include "candy/utils/Options.h"
#include "candy/core/SolverTypes.h"
#include "candy/mtl/BoundedQueue.h"
#include "candy/core/Clause.h"
#include "candy/core/Certificate.h"
#include "candy/utils/CNFProblem.h"

#include "sonification/SolverSonification.h"

namespace Candy {

using namespace std;

class Solver {

    friend class SolverConfiguration;

public:
    Solver();
    virtual ~Solver();

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

    // FALSE means solver is in a conflicting state
    inline bool okay() const {
        return ok;
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
    bool sort_learnts;

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

	// Main internal methods:
    inline Clause* reason(Var x) const {
        return vardata[x].reason;
    }

    inline int level(Var x) const {
        return vardata[x].level;
    }

    inline uint64_t abstractLevel(Var x) const {
        return 1 << (level(x) & 63);
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
	        for (size_t i = 0; i < nVars(); i++) {
	            activity[i] *= 1e-100; // rescale
	        }
	        var_inc *= 1e-100;
	    }
	    if (order_heap.inHeap(v)) {
	        order_heap.decrease(v); // update order-heap
	    }
	}

	inline void claDecayActivity() {
	    cla_inc *= (1 / clause_decay);
	}

	inline void claBumpActivity(Clause& c) {
	    if ((c.activity() += cla_inc) > 1e20) {
	        for (Clause* clause : clauses)
	            clause->activity() *= 1e-20;
	        for (Clause* clause : learnts)
	            clause->activity() *= 1e-20;
	        for (Clause* clause : learntsBin)
	            clause->activity() *= 1e-20;
	        cla_inc *= 1e-20;
	    }
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

}

#endif
