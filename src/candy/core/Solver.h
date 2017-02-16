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

#include "candy/mtl/Heap.h"
#include "candy/utils/Options.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/BoundedQueue.h"
#include "candy/core/Constants.h"
#include <vector>
#include "candy/core/Clause.h"

#include "CNFProblem.h"

#include "sonification/SolverSonification.h"

namespace Glucose {

using namespace std;

//=================================================================================================
// Solver -- the main class:

class Solver {

	friend class SolverConfiguration;

public:

	// Constructor/Destructor:
	//
	Solver();
	virtual ~Solver();

	// Problem specification:
	//
	virtual Var newVar(bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.
	bool addClause(const vector<Lit>& ps); // Add a clause to the solver.
	bool addEmptyClause(); // Add the empty clause, making the solver contradictory.
	bool addClause(Lit p); // Add a unit clause to the solver.
	bool addClause(Lit p, Lit q); // Add a binary clause to the solver.
	bool addClause(Lit p, Lit q, Lit r); // Add a ternary clause to the solver.
	virtual bool addClause_(vector<Lit>& ps); // Add a clause to the solver without making superflous internal copy. Will change ps
	void addClauses(Candy::CNFProblem dimacs);

	// use with care (written for solver tests only)
	Candy::Clause& getClause(unsigned int pos) {
		assert(pos < clauses.size());
		return *clauses[pos];
	}

	// Solving:
	//
	bool simplify();                       // Removes already satisfied clauses.
	bool solve(const vector<Lit>& assumps); // Search for a model that respects a given set of assumptions.
	lbool solveLimited(const vector<Lit>& assumps); // Search for a model that respects a given set of assumptions (With resource constraints).
	bool solve();                        // Search without assumptions.
	bool solve(Lit p);  // Search for a model that respects a single assumption.
	bool solve(Lit p, Lit q); // Search for a model that respects two assumptions.
	bool solve(Lit p, Lit q, Lit r); // Search for a model that respects three assumptions.
	bool okay() const;           // FALSE means solver is in a conflicting state

	// Display clauses and literals (Debug purpose)
	void printLit(Lit l) {
		if (status == l_True) {
			printf("%s%d:%c", sign(l) ? "-" : "", var(l) + 1,
					modelValue(l) == l_True ?
							'1' : (modelValue(l) == l_False ? '0' : 'X'));
		} else {
			printf("%s%d:%c", sign(l) ? "-" : "", var(l) + 1,
					value(l) == l_True ?
							'1' : (value(l) == l_False ? '0' : 'X'));
		}
	}

	void printClause(Candy::Clause& c, bool skipSelectorVariables = false) {
		for (int i = 0; i < c.size(); i++) {
			if (!skipSelectorVariables || !isSelector(var(c[i]))) {
				printLit(c[i]);
				printf(" ");
			}
		}
	}

	void printProblem(bool skipSelectorVariables = false) {
		for (auto clause : clauses) {
			printClause(*clause, skipSelectorVariables);
			printf("\n");
		}
	}

	// Variable mode:
	//
	void setPolarity(Var v, bool b); // Declare which polarity the decision heuristic should use for a variable. Requires mode 'polarity_user'.
	void setDecisionVar(Var v, bool b); // Declare if a variable should be eligible for selection in the decision heuristic.

	// Read state:
	//
	lbool value(Var x) const;       // The current value of a variable.
	lbool value(Lit p) const;       // The current value of a literal.
	lbool modelValue(Var x) const; // The value of a variable in the last model. The last call to solve must have been satisfiable.
	lbool modelValue(Lit p) const; // The value of a literal in the last model. The last call to solve must have been satisfiable.
	int nAssigns() const;       // The current number of assigned literals.
	int nClauses() const;       // The current number of original clauses.
	int nLearnts() const;       // The current number of learnt clauses.
	int nVars() const;       // The current number of variables.
	int nFreeVars() const;

	inline char valuePhase(Var v) {
		return polarity[v];
	}

	// Incremental mode
	void setIncrementalMode();
	void initNbInitialVars(int nb);
	void printIncrementalStats();
	bool isIncremental();

	// Resource contraints:
	//
	void setConfBudget(int64_t x);
	void setPropBudget(int64_t x);
	void budgetOff();
	void interrupt(); // Trigger a (potentially asynchronous) interruption of the solver.
	void clearInterrupt();     // Clear interrupt indicator flag.

	// Extra results: (read-only member variable)
	//
	vector<lbool> model; // If problem is satisfiable, this vector contains the model (if any).
	vector<Lit> conflict; // If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.

	// Mode of operation:
	//
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

	// Constant for Memory managment
	double garbage_frac; // The fraction of wasted memory allowed before a garbage collection is triggered.

	// Certified UNSAT ( Thanks to Marijn Heule)
	FILE* certifiedOutput;
	bool certifiedUNSAT;

	// Statistics: (read-only member variable)
	uint64_t originalClausesSeen; // Number of original clauses seen
	uint64_t sumDecisionLevels;
	//
	uint64_t nbRemovedClauses, nbReducedClauses, nbDL2, nbBin, nbUn, nbReduceDB,
			solves, starts, decisions, rnd_decisions, propagations, conflicts,
			conflictsRestarts, nbstopsrestarts, nbstopsrestartssame,
			lastblockatrestart;
	uint64_t dec_vars, clauses_literals, learnts_literals, max_literals,
			tot_literals;

	void setTermCallback(void* state, int (*termCallback)(void*)) {
		this->termCallbackState = state;
		this->termCallback = termCallback;
	}

protected:

	long curRestart;

	// Helper structures:
	struct VarData {
	    Candy::Clause* reason;
		unsigned int level;
	};

	static inline VarData mkVarData(Candy::Clause* cr, unsigned int l) {
		VarData d = { cr, l };
		return d;
	}

	struct Watcher {
	    Candy::Clause* cref;
		Lit blocker;
		Watcher() :
				cref(0), blocker(lit_Undef) {
		}
		Watcher(Candy::Clause* cr, Lit p) :
				cref(cr), blocker(p) {
		}
		bool operator==(const Watcher& w) const {
			return cref == w.cref;
		}
		bool operator!=(const Watcher& w) const {
			return cref != w.cref;
		}
	};

	struct WatcherDeleted {
		WatcherDeleted() { }
		bool operator()(const Watcher& w) const {
			return w.cref->mark() == 1;
		}
	};

	struct VarOrderLt {
		const vector<double>& activity;
		bool operator()(Var x, Var y) const {
			return activity[x] > activity[y];
		}
		VarOrderLt(const vector<double>& act) :
				activity(act) {
		}
	};

	// Solver state:
	bool ok; // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!
	double cla_inc;          // Amount to bump next clause with.
	vector<double> activity; // A heuristic measurement of the activity of a variable.
	double var_inc;          // Amount to bump next variable with.
	OccLists<Lit, Watcher, WatcherDeleted> watches; // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
	OccLists<Lit, Watcher, WatcherDeleted> watchesBin; // 'watches[lit]' is a list of constraints watching 'lit' (will go there if literal becomes true).
	vector<Candy::Clause*> clauses;          // List of problem clauses.
	vector<Candy::Clause*> learnts;          // List of learnt clauses.

	vector<lbool> assigns;          // The current assignments.
	vector<char> polarity;         // The preferred polarity of each variable.
	vector<char> decision; // Declares if a variable is eligible for selection in the decision heuristic.
	vector<Lit> trail; // Assignment stack; stores all assigments made in the order they were made.
	int trail_size; // Current number of assignments (used to optimize propagate, through getting rid of capacity checking)
	vector<int> nbpos;
	vector<int> trail_lim; // Separator indices for different decision levels in 'trail'.
	vector<VarData> vardata;       // Stores reason and level for each variable.
	int qhead; // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
	int simpDB_assigns; // Number of top-level assignments since last execution of 'simplify()'.
	int64_t simpDB_props; // Remaining number of propagations that must be made before next execution of 'simplify()'.
	vector<Lit> assumptions; // Current set of assumptions provided to solve by the user.
	Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable activity.
	bool remove_satisfied; // Indicates whether possibly inefficient linear scan for satisfied clauses should be performed in 'simplify'.
	bool reduceOnSize;
	int reduceOnSizeSize;                // See XMinisat paper
	vector<unsigned int> permDiff; // permDiff[var] contains the current conflict number... Used to count the number of  LBD

	// UPDATEVARACTIVITY trick (see competition'09 companion paper)
	vector<Lit> lastDecisionLevel;

	int nbclausesbeforereduce; // To know when it is time to reduce clause database

	// Used for restart strategies
	bqueue<unsigned int> trailQueue, lbdQueue; // Bounded queues for restarts.
	float sumLBD; // used to compute the global average of LBD. Restarts...
	//int sumAssumptions;
	Candy::Clause* lastLearntClause;

	// Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
	// used, exept 'seen' wich is used in several places.
	//
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
	double totalTime4Sat, totalTime4Unsat;
	int nbSatCalls, nbUnsatCalls;

	SolverSonification sonification;

	void* termCallbackState;
	int (*termCallback)(void* state);

	lbool status;

	// Main internal methods:
	//
	void insertVarOrder(Var x); // Insert a variable in the decision order priority queue.
	Lit pickBranchLit(); // Return the next decision variable.
	void newDecisionLevel(); // Begins a new decision level.
	void uncheckedEnqueue(Lit p, Candy::Clause* from = nullptr); // Enqueue a literal. Assumes value of literal is undefined.
	bool enqueue(Lit p, Candy::Clause* from = nullptr); // Test if fact 'p' contradicts current state, enqueue otherwise.
	Candy::Clause* propagate(); // Perform unit propagation. Returns possibly conflicting clause.
	void cancelUntil(int level); // Backtrack until a certain level.
	void analyze(Candy::Clause* confl, vector<Lit>& out_learnt, vector<Lit>& selectors,
			int& out_btlevel, unsigned int &nblevels,
			unsigned int &szWithoutSelectors); // (bt = backtrack)
	void analyzeFinal(Lit p, vector<Lit>& out_conflict); // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
	bool litRedundant(Lit p, uint32_t abstract_levels); // (helper method for 'analyze()')
	lbool search(int nof_conflicts); // Search for a given number of conflicts.
	virtual lbool solve_(bool do_simp = true, bool turn_off_simp = false); // Main solve method (assumptions given in 'assumptions').
	virtual void reduceDB(); // Reduce the set of learnt clauses.
	void removeSatisfied(vector<Candy::Clause*>& cs); // Shrink 'cs' to contain only non-satisfied clauses.
	void rebuildOrderHeap();

	// Maintaining Variable/Clause activity:
	//
	void varDecayActivity(); // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
	void varBumpActivity(Var v, double inc); // Increase a variable with the current 'bump' value.
	void varBumpActivity(Var v); // Increase a variable with the current 'bump' value.
	void claDecayActivity(); // Decay all clauses with the specified factor. Implemented by increasing the 'bump' value instead.
	void claBumpActivity(Candy::Clause& c); // Increase a clause with the current 'bump' value.

	// Operations on clauses:
	//
	void attachClause(Candy::Clause* cr); // Attach a clause to watcher lists.
	void detachClause(Candy::Clause* cr, bool strict = false); // Detach a clause to watcher lists.
	void removeClause(Candy::Clause* cr); // Detach and free a clause.
	bool locked(Candy::Clause* c) const; // Returns TRUE if a clause is a reason for some implication in the current state.
	bool satisfied(const Candy::Clause& c) const; // Returns TRUE if a clause is satisfied in the current state.

	unsigned int computeLBD(const vector<Lit>& lits, int end = -1);
	unsigned int computeLBD(const Candy::Clause &c);
	void minimisationWithBinaryResolution(vector<Lit> &out_learnt);

	// Misc:
	//
	int decisionLevel() const; // Gives the current decisionlevel.
	uint32_t abstractLevel(Var x) const; // Used to represent an abstraction of sets of decision levels.
	Candy::Clause* reason(Var x) const;
	int level(Var x) const;
	bool withinBudget() const;
	inline bool isSelector(Var v) {
		return (incremental && v >= nbVarsInitialFormula);
	}

	// Static helpers:
	//
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

//=================================================================================================
// Implementation of inline methods:

inline Candy::Clause* Solver::reason(Var x) const {
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
	varBumpActivity(v, var_inc);
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
inline void Solver::claBumpActivity(Candy::Clause& c) {
	if ((c.activity() += cla_inc) > 1e20) {
		// Rescale:
		for (Candy::Clause* clause : learnts)
			clause->activity() *= 1e-20;
		cla_inc *= 1e-20;
	}
}

// NOTE: enqueue does not set the ok flag! (only public methods do)
inline bool Solver::enqueue(Lit p, Candy::Clause* from) {
	return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true);
}
inline bool Solver::addClause(const vector<Lit>& ps) {
	add_tmp.clear();
	add_tmp.insert(add_tmp.end(), ps.begin(), ps.end());
	return addClause_(add_tmp);
}
inline bool Solver::addEmptyClause() {
	add_tmp.clear();
	return addClause_(add_tmp);
}
inline bool Solver::addClause(Lit p) {
	add_tmp.clear();
	add_tmp.push_back(p);
	return addClause_(add_tmp);
}
inline bool Solver::addClause(Lit p, Lit q) {
	add_tmp.clear();
	add_tmp.push_back(p);
	add_tmp.push_back(q);
	return addClause_(add_tmp);
}
inline bool Solver::addClause(Lit p, Lit q, Lit r) {
	add_tmp.clear();
	add_tmp.push_back(p);
	add_tmp.push_back(q);
	add_tmp.push_back(r);
	return addClause_(add_tmp);
}

// TODO: optimize conditions
inline bool Solver::locked(Candy::Clause* cr) const {
    Candy::Clause& c = *cr;
	if (c.size() > 2)
		return value(c[0]) == l_True && reason(var(c[0])) != nullptr
		                && reason(var(c[0])) == cr;
	return (value(c[0]) == l_True && reason(var(c[0])) != nullptr
	                && reason(var(c[0])) == cr)
			|| (value(c[1]) == l_True && reason(var(c[1])) != nullptr
					&& reason(var(c[1])) == cr);
}

inline void Solver::newDecisionLevel() {
	trail_lim.push_back(trail_size);
}

inline int Solver::decisionLevel() const {
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
	return learnts.size();
}
inline int Solver::nVars() const {
	return vardata.size();
}
inline int Solver::nFreeVars() const {
	return (int) dec_vars - (trail_lim.size() == 0 ? trail_size : trail_lim[0]);
}
inline void Solver::setPolarity(Var v, bool b) {
	polarity[v] = b;
}
inline void Solver::setDecisionVar(Var v, bool b) {
	if (b && !decision[v])
		dec_vars++;
	else if (!b && decision[v])
		dec_vars--;

	decision[v] = b;
	insertVarOrder(v);
}
inline void Solver::setConfBudget(int64_t x) {
	conflict_budget = conflicts + x;
}
inline void Solver::setPropBudget(int64_t x) {
	propagation_budget = propagations + x;
}
inline void Solver::interrupt() {
	asynch_interrupt = true;
}
inline void Solver::clearInterrupt() {
	asynch_interrupt = false;
}
inline void Solver::budgetOff() {
	conflict_budget = propagation_budget = -1;
}
inline bool Solver::withinBudget() const {
	return !asynch_interrupt
			&& (termCallback == nullptr || 0 == termCallback(termCallbackState))
			&& (conflict_budget < 0 || conflicts < (uint64_t) conflict_budget)
			&& (propagation_budget < 0
					|| propagations < (uint64_t) propagation_budget);
}

// FIXME: after the introduction of asynchronous interrruptions the solve-versions that return a
// pure bool do not give a safe interface. Either interrupts must be possible to turn off here, or
// all calls to solve must return an 'lbool'. I'm not yet sure which I prefer.
inline bool Solver::solve() {
	budgetOff();
	assumptions.clear();
	return solve_() == l_True;
}
inline bool Solver::solve(Lit p) {
	budgetOff();
	assumptions.clear();
	assumptions.push_back(p);
	return solve_() == l_True;
}
inline bool Solver::solve(Lit p, Lit q) {
	budgetOff();
	assumptions.clear();
	assumptions.push_back(p);
	assumptions.push_back(q);
	return solve_() == l_True;
}
inline bool Solver::solve(Lit p, Lit q, Lit r) {
	budgetOff();
	assumptions.clear();
	assumptions.push_back(p);
	assumptions.push_back(q);
	assumptions.push_back(r);
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

//=================================================================================================

struct reduceDB_lt {
	reduceDB_lt() { }

	bool operator()(Candy::Clause* x, Candy::Clause* y) {
		// Main criteria... Like in MiniSat we keep all binary clauses
		if (x->size() > 2 && y->size() == 2)
			return 1;

		if (y->size() > 2 && x->size() == 2)
			return 0;
		if (x->size() == 2 && y->size() == 2)
			return 0;

		// Second one  based on literal block distance
		if (x->lbd() > y->lbd())
			return 1;
		if (x->lbd() < y->lbd())
			return 0;

		// Finally we can use old activity or size, we choose the last one
		return x->activity() < y->activity();
		//return x->size() < y->size();
		//return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity()); }
	}
};

}

#endif
