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

#include <core/Statistics.h>
#include "candy/mtl/Heap.h"
#include "candy/utils/Options.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/BoundedQueue.h"
#include "candy/core/Constants.h"
#include <vector>
#include "candy/core/Clause.h"
#include "candy/core/Certificate.h"
#include "CNFProblem.h"

#include "sonification/SolverSonification.h"

namespace Candy {

using namespace std;

class Solver {

    friend class SolverConfiguration;

public:
    Solver();
    virtual ~Solver();

    // Problem specification:
    virtual Var newVar(bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.
    virtual bool addClause_(vector<Lit>& ps); // Add a clause to the solver without making superflous internal copy. Will change ps
    bool addClause(const vector<Lit>& ps); // Add a clause to the solver.
    bool addClause(std::initializer_list<Lit> lits);
    void addClauses(CNFProblem dimacs);

    // use with care (written for solver tests only)
    Clause& getClause(unsigned int pos) {
        assert(pos < clauses.size());
        return *clauses[pos];
    }

    // Solving:
    bool simplify();                       // Removes already satisfied clauses.
    bool solve(const vector<Lit>& assumps); // Search for a model that respects a given set of assumptions.
    lbool solveLimited(const vector<Lit>& assumps); // Search for a model that respects a given set of assumptions (With resource constraints).
    bool solve(std::initializer_list<Lit> assumps);
    bool okay() const;           // FALSE means solver is in a conflicting state

    // Variable mode:
    void setPolarity(Var v, bool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
    void setDecisionVar(Var v, bool b); // Declare if a variable should be eligible for selection in the decision heuristic.

    // Read state:
    lbool value(Var x) const;       // The current value of a variable.
    lbool value(Lit p) const;       // The current value of a literal.
    lbool modelValue(Var x) const; // The value of a variable in the last model. The last call to solve must have been satisfiable.
    lbool modelValue(Lit p) const; // The value of a literal in the last model. The last call to solve must have been satisfiable.
    int nAssigns() const;       // The current number of assigned literals.
    int nClauses() const;       // The current number of original clauses.
    int nLearnts() const;       // The current number of learnt clauses.
    int nVars() const;       // The current number of variables.

    inline char valuePhase(Var v) {
        return polarity[v];
    }

    // Incremental mode
    void setIncrementalMode();
    void initNbInitialVars(int nb);
    bool isIncremental();

    // Resource contraints:
    void setConfBudget(int64_t x);
    void setPropBudget(int64_t x);
    void budgetOff();
    void setInterrupt(bool value = true); // Trigger a (potentially asynchronous) interruption of the solver.

    //TODO: use std::function<int(void*)> as type here
    void setTermCallback(void* state, int (*termCallback)(void*)) {
        this->termCallbackState = state;
        this->termCallback = termCallback;
    }

    // Extra results: (read-only member variable)
    vector<lbool> model; // If problem is satisfiable, this vector contains the model (if any).
    vector<Lit> conflict; // If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.

    // Mode of operation:
    int verbosity;
    int verbEveryConflicts;
    int showModel;  // deprecated: can go to main

    // Constants For restarts
    double K;
    double R;
    double sizeLBDQueue;
    double sizeTrailQueue;

    // Constants for reduce DB
    int incReduceDB;
    int specialIncReduceDB;
    unsigned int lbLBDFrozenClause;

    // Constant for reducing clause
    int lbSizeMinimizingClause;
    unsigned int lbLBDMinimizingClause;

    // Constant for heuristic
    double var_decay;
    double max_var_decay;
    double clause_decay;
    double random_var_freq;
    double random_seed;
    int ccmin_mode; // Controls conflict clause minimization (0=none, 1=basic, 2=deep).
    int phase_saving; // Controls the level of phase saving (0=none, 1=limited, 2=full).
    bool rnd_pol;            // Use random polarities for branching heuristics.
    bool rnd_init_act; // Initialize variable activities with a small random value.

    // Certified UNSAT ( Thanks to Marijn Heule)
    Certificate certificate;
    // a few stats are used for heuristics control, keep them here:
    uint64_t nConflicts, nPropagations, nLiterals;

protected:
	long curRestart;

	// Helper structures:
	struct VarData {
	    Clause* reason;
		unsigned int level;
		VarData() :
		    reason(nullptr), level(0) {}
		VarData(Clause* _reason, unsigned int _level) :
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

	// Solver state:
	bool ok; // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
	double cla_inc;          // Amount to bump next clause with.
	vector<double> activity; // A heuristic measurement of the activity of a variable.
	double var_inc;          // Amount to bump next variable with.
	OccLists<Lit, Watcher, WatcherDeleted> watches; // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
	OccLists<Lit, Watcher, WatcherDeleted> watchesBin; // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
	vector<Clause*> clauses;          // List of problem clauses.
	vector<Clause*> learnts;          // List of learnt clauses.
	vector<Clause*> learntsBin;       // List of binary learnt clauses.

	vector<lbool> assigns;          // The current assignments.
	vector<char> polarity;         // The preferred polarity of each variable.
	vector<char> decision; // Declares if a variable is eligible for selection in the decision heuristic.
	vector<Lit> trail; // Assignment stack; stores all assigments made in the order they were made.
	int trail_size; // Current number of assignments (used to optimize propagate, through getting rid of capacity checking)
	vector<int> trail_lim; // Separator indices for different decision levels in 'trail'.
	vector<VarData> vardata;       // Stores reason and level for each variable.
	int qhead; // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
	int simpDB_assigns; // Number of top-level assignments since last execution of 'simplify()'.
	int64_t simpDB_props; // Remaining number of propagations that must be made before next execution of 'simplify()'.
	vector<Lit> assumptions; // Current set of assumptions provided to solve by the user.
	Glucose::Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable activity.
	bool remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.

	// See XMinisat paper: prefer size over LBD for small clauses in reduceDB
	bool reduceOnSize;
	int reduceOnSizeSize;

	vector<unsigned int> permDiff; // permDiff[var] contains the current conflict number... Used to count the number of  LBD

	// UPDATEVARACTIVITY trick (see competition'09 companion paper)
	vector<Lit> lastDecisionLevel;

	int nbclausesbeforereduce; // To know when it is time to reduce clause database

	// Used for restart strategies
	Glucose::bqueue<unsigned int> trailQueue, lbdQueue; // Bounded queues for restarts.
	float sumLBD; // used to compute the global average of LBD. Restarts...

	// Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
	// used, exept 'seen' wich is used in several places.
	vector<char> seen;
	vector<Lit> analyze_stack;
	vector<Lit> analyze_toclear;
	vector<Lit> add_tmp;
	unsigned int MYFLAG;

	// Resource contraints:
	int64_t conflict_budget;    // -1 means no budget.
	int64_t propagation_budget; // -1 means no budget.
	bool asynch_interrupt;

	// Variables added for incremental mode
	int incremental; // Use incremental SAT Solver
	int nbVarsInitialFormula; // nb VAR in formula without assumptions (incremental SAT)

	SolverSonification sonification;

	void* termCallbackState;
	int (*termCallback)(void* state);

	lbool status;

	// Main internal methods:
	void insertVarOrder(Var x); // Insert a variable in the decision order priority queue.
	Lit pickBranchLit(); // Return the next decision variable.
	void newDecisionLevel(); // Begins a new decision level.
	void uncheckedEnqueue(Lit p, Clause* from = nullptr); // Enqueue a literal. Assumes value of literal is undefined.
	bool enqueue(Lit p, Clause* from = nullptr); // Test if fact 'p' contradicts current state, enqueue otherwise.
	Clause* propagate(); // Perform unit propagation. Returns possibly conflicting clause.
	void cancelUntil(int level); // Backtrack until a certain level.
	void analyze(Clause* confl, vector<Lit>& out_learnt, int& out_btlevel, unsigned int &nblevels); // (bt = backtrack)
	void analyzeFinal(Lit p, vector<Lit>& out_conflict); // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
	bool seenAny(Clause& clause);
	bool litRedundant(Lit p, uint32_t abstract_levels); // (helper method for 'analyze()')
	lbool search(); // Search for a given number of conflicts.
	virtual lbool solve_(bool do_simp = true, bool turn_off_simp = false); // Main solve method (assumptions given in 'assumptions').
	virtual void reduceDB(); // Reduce the set of learnt clauses.
	void rebuildOrderHeap();
    void revampClausePool();

	// Maintaining Variable/Clause activity:
	void varDecayActivity(); // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
	void varBumpActivity(Var v, double inc); // Increase a variable with the current 'bump' value.
	void varBumpActivity(Var v); // Increase a variable with the current 'bump' value.
	void claDecayActivity(); // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.
	void claBumpActivity(Clause& c); // Increase a clause with the current 'bump' value.

	// Operations on clauses:
	void attachClause(Clause* cr); // Attach a clause to watcher lists.
	void detachClause(Clause* cr, bool strict = false); // Detach a clause to watcher lists.
	void removeClause(Clause* cr); // Detach and free a clause.
	void freeMarkedClauses(vector<Clause*>& list);
	bool locked(Clause* c) const; // Returns TRUE if a clause is a reason for some implication in the current state.
	bool satisfied(const Clause& c) const; // Returns TRUE if a clause is satisfied in the current state.

	template <typename Iterator>
	unsigned int computeLBD(Iterator it, Iterator end);
	void minimisationWithBinaryResolution(vector<Lit> &out_learnt);

	// Misc:
	uint32_t decisionLevel() const; // Gives the current decisionlevel.
	uint32_t abstractLevel(Var x) const; // Used to represent an abstraction of sets of decision levels.
	Clause* reason(Var x) const;
	int level(Var x) const;
	bool withinBudget();
	inline bool isSelector(Var v) {
		return (incremental && v >= nbVarsInitialFormula);
	}

	// Static helpers:
	// Returns a random float 0 <= x < 1. Seed must never be 0.
	static inline double drand(double& seed) {
		seed *= 1389796;
		int q = (int) (seed / 2147483647);
		seed -= (double) q * 2147483647;
		return seed / 2147483647;
	}

	// Returns a random integer 0 <= x < size. Seed must never be 0.
	static inline int irand(double& seed, int size) {
		return (int) (drand(seed) * size);
	}
};


// Implementation of inline methods:
inline Clause* Solver::reason(Var x) const {
    return vardata[x].reason;
}

inline int Solver::level(Var x) const {
    return vardata[x].level;
}

inline void Solver::insertVarOrder(Var x) {
    if (!order_heap.inHeap(x) && decision[x])
        order_heap.insert(x);
}

inline void Solver::varDecayActivity() {
    var_inc *= (1 / var_decay);
}
inline void Solver::varBumpActivity(Var v) {
    if (!isSelector(v)) {
        varBumpActivity(v, var_inc);
    }
}
inline void Solver::varBumpActivity(Var v, double inc) {
    if ((activity[v] += inc) > 1e100) {
        // Rescale:
        for (int i = 0; i < nVars(); i++)
            activity[i] *= 1e-100;
        var_inc *= 1e-100;
    }
    // Update order_heap with respect to new activity:
    if (order_heap.inHeap(v))
        order_heap.decrease(v);
}

inline void Solver::claDecayActivity() {
	cla_inc *= (1 / clause_decay);
}
inline void Solver::claBumpActivity(Clause& c) {
	if ((c.activity() += cla_inc) > 1e20) {
		// Rescale:
        for (Clause* clause : clauses)
            clause->activity() *= 1e-20;
		for (Clause* clause : learnts)
			clause->activity() *= 1e-20;
        for (Clause* clause : learntsBin)
            clause->activity() *= 1e-20;
		cla_inc *= 1e-20;
	}
}

// NOTE: enqueue does not set the ok flag! (only public methods do)
inline bool Solver::enqueue(Lit p, Clause* from) {
	return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true);
}
inline bool Solver::addClause(const vector<Lit>& ps) {
    add_tmp.clear();
    add_tmp.insert(add_tmp.end(), ps.begin(), ps.end());
    return addClause_(add_tmp);
}
inline bool Solver::addClause(std::initializer_list<Lit> lits) {
    add_tmp.clear();
    add_tmp.insert(add_tmp.end(), lits.begin(), lits.end());
    return addClause_(add_tmp);
}

inline bool Solver::locked(Clause* cr) const {
    Clause& c = *cr;
    if (c.size() > 2) return value(c[0]) == l_True && reason(var(c[0])) == cr;
    return (value(c[0]) == l_True && reason(var(c[0])) == cr) || (value(c[1]) == l_True && reason(var(c[1])) == cr);
}

inline void Solver::newDecisionLevel() {
    trail_lim.push_back(trail_size);
}

inline uint32_t Solver::decisionLevel() const {
    return trail_lim.size();
}
inline uint32_t Solver::abstractLevel(Var x) const {
    return 1 << (level(x) & 31);
}
inline lbool Solver::value(Var x) const {
    return assigns[x];
}
inline lbool Solver::value(Lit p) const {
    return assigns[var(p)] ^ sign(p);
}
inline lbool Solver::modelValue(Var x) const {
    return model[x];
}
inline lbool Solver::modelValue(Lit p) const {
    return model[var(p)] ^ sign(p);
}
inline int Solver::nAssigns() const {
    return trail_size;
}
inline int Solver::nClauses() const {
    return clauses.size();
}
inline int Solver::nLearnts() const {
    return learnts.size() + learntsBin.size();
}
inline int Solver::nVars() const {
    return vardata.size();
}
inline void Solver::setPolarity(Var v, bool b) {
    polarity[v] = b;
}
inline void Solver::setDecisionVar(Var v, bool b) {
    if (b && !decision[v])
        Statistics::getInstance().solverDecisionVariablesInc();
    else if (!b && decision[v])
        Statistics::getInstance().solverDecisionVariablesDec();

    decision[v] = b;
    insertVarOrder(v);
}
inline void Solver::setConfBudget(int64_t x) {
    conflict_budget = nConflicts + x;
}
inline void Solver::setPropBudget(int64_t x) {
    propagation_budget = nPropagations + x;
}
inline void Solver::setInterrupt(bool value) {
    asynch_interrupt = value;
}
inline void Solver::budgetOff() {
    conflict_budget = propagation_budget = -1;
}
inline bool Solver::withinBudget() {
    return !asynch_interrupt && (termCallback == nullptr || 0 == termCallback(termCallbackState))
            && (conflict_budget < 0 || nConflicts < (uint64_t)conflict_budget) && (propagation_budget < 0 || nPropagations < (uint64_t)propagation_budget);
}

// FIXME: after the introduction of asynchronous interrruptions the solve-versions that return a
// pure bool do not give a safe interface. Either interrupts must be possible to turn off here, or
// all calls to solve must return an 'lbool'. I'm not yet sure which I prefer.
inline bool Solver::solve(std::initializer_list<Lit> assumps) {
    budgetOff();
    assumptions.clear();
    assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
    return solve_() == l_True;
}
inline bool Solver::solve(const vector<Lit>& assumps) {
    budgetOff();
    assumptions.clear();
    assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
    return solve_() == l_True;
}
inline lbool Solver::solveLimited(const vector<Lit>& assumps) {
    assumptions.clear();
    assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
    return solve_();
}
inline bool Solver::okay() const {
    return ok;
}

struct reduceDB_lt {
    reduceDB_lt() {
    }

    bool operator()(Clause* x, Clause* y) {
//        if (reduceOnSize) {
//            int lbd1 = x->size() < reduceOnSizeSize : x->size() : x->size() + x->getLBD();
//            int lbd2 = y->size() < reduceOnSizeSize : y->size() : y->size() + y->getLBD();
//            return lbd1 > lbd2 || (lbd1 == lbd2 && x->activity() < y->activity());
//        }
        return x->getLBD() > y->getLBD() || (x->getLBD() == y->getLBD() && x->activity() < y->activity());
    }
};

}

#endif
