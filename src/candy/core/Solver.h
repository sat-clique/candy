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
#include <unordered_map>
#include <math.h>
#include <string>
#include <type_traits>
#include <memory>
#include <limits>

#include "candy/core/Statistics.h"
#include "candy/mtl/Heap.h"
#include "candy/frontend/CLIOptions.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/ClauseDatabase.h"
#include "candy/mtl/BoundedQueue.h"
#include "candy/core/Clause.h"
#include "candy/core/Certificate.h"
#include "candy/core/ClauseAllocator.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/ConflictAnalysis.h"
#include "candy/utils/System.h"
#include "candy/utils/Attributes.h"
#include "candy/utils/CheckedCast.h"
#include "candy/core/Trail.h"
#include "candy/core/Propagate.h"
#include "candy/core/branching/VSIDS.h"
#include "candy/core/Stamp.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/randomsimulation/Conjectures.h"
#include "candy/rsar/Refinement.h"

#include "candy/sonification/SolverSonification.h"
#include "candy/sonification/ControllerInterface.h"

namespace Candy {

/**
 * \tparam TBranching   the PickBranchLit type used to choose a
 *   strategy for determining decision (ie. branching) literals.
 *   TBranching must satisfy the following conditions:
 *    - TBranching must be a class type.
 *    - TBranching::Parameters must be a class type.
 *    - TBranching must have a zero-argument constructor.
 *    - TBranching must have a constructor taking a single argument of type
 *        const Parameters& params.
 *    - TBranching must be move-assignable.
 *    - TBranching must be move-constructible.
 *    - There must be a specialization of Solver::pickBranchLit<TBranching>.
 */
template<class TClauseDatabase = ClauseDatabase, class TAssignment = Trail, class TPropagate = Propagate, class TLearning = ConflictAnalysis, class TBranching = VSIDS>
class Solver : public CandySolverInterface {
    static_assert(std::is_class<TBranching>::value, "TBranching must be a class");
    //static_assert(std::is_constructible<TBranching>::value, "TBranching must have a constructor without arguments");
    //static_assert(std::is_move_assignable<TBranching>::value, "TBranching must be move-assignable");
    //static_assert(std::is_move_constructible<TBranching>::value, "TBranching must be move-constructible");

    friend class SolverConfiguration;

public:
    Solver();
    Solver(TClauseDatabase* _clause_db);
    Solver(TClauseDatabase& db, TAssignment& as, TPropagate& pr, TLearning& le, TBranching& br);
    virtual ~Solver();
    
    Var newVar() override;
    void addClauses(const CNFProblem& problem) override;

    template<typename Iterator>
    bool addClause(Iterator begin, Iterator end, unsigned int lbd = 0);

    bool addClause(const std::vector<Lit>& lits, unsigned int lbd = 0) override { 
        return addClause(lits.begin(), lits.end(), lbd);
    }

    bool addClause(std::initializer_list<Lit> lits, unsigned int lbd = 0) override {
        return addClause(lits.begin(), lits.end(), lbd);
    }

    void printDIMACS() override {
        printf("p cnf %zu %zu\n", nVars(), nClauses());
        for (auto clause : clause_db.clauses) {
            clause->printDIMACS();
        }
    }

    vector<Lit>& getConflict() override {
        return conflict;
    }

    // Solving:
    void simplify() override; // remove satisfied clauses 
    void strengthen() override; // remove false literals from clauses
    virtual bool eliminate() override { return true; } // Perform variable elimination based simplification.
    virtual bool eliminate(bool use_asymm, bool use_elim) override { return true; } // Perform variable elimination based simplification.
    
    void enablePreprocessing() override {
        preprocessing_enabled = true;
    }

    void disablePreprocessing() override {
        preprocessing_enabled = false;
    }

    virtual bool isEliminated(Var v) const override { return false; }

    void setFrozen(Var v, bool freeze) override  {
        if (freeze) {
            freezes.push_back(v);
        } else {
            freezes.erase(std::remove(freezes.begin(), freezes.end(), v), freezes.end());
        }
    }

    virtual lbool solve() override; // Main solve method (assumptions given in 'assumptions').

    lbool solve(std::initializer_list<Lit> assumps) override {
        assumptions.clear();
        assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
        return solve();
    }

    lbool solve(const vector<Lit>& assumps) override {
        assumptions.clear();
        assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
        return solve();
    }

    // true means solver is in a conflicting state
    bool isInConflictingState() const override {
        return !ok;
    }

    // The value of a variable in the last model. The last call to solve must have been satisfiable.
    lbool modelValue(Var x) const override {
        return model[x];
    }
    // The value of a literal in the last model. The last call to solve must have been satisfiable.
    lbool modelValue(Lit p) const override {
        return model[var(p)] ^ sign(p);
    }
    // create a list of literals that reflects the current assignment
    Cl getModel() override {
        Cl literals;
        literals.resize(nVars());
        for (unsigned int v = 0; v < nVars(); v++) {
            literals[v] = model[v] == l_True ? mkLit(v, false) : model[v] == l_False ? mkLit(v, true) : lit_Undef;
        }
        return literals;
    }
    size_t nClauses() const override {
        return clause_db.clauses.size();
    }
    size_t nVars() const override {
        return trail.vardata.size();
    }
    size_t nConflicts() const override {
        return clause_db.getConflictResult().nConflicts;
    }
    size_t nPropagations() const override {
        return propagator.nPropagations;
    }

    bool isSelector(Var v) {
        return false;
    }

    // Incremental mode
    void setIncrementalMode() override;
    bool isIncremental() override;

    // Resource constraints:
    void setConfBudget(uint64_t x) override {
        conflict_budget = nConflicts() + x;
    }
    void setPropBudget(uint64_t x) override {
        propagation_budget = nPropagations() + x;
    }
    void setInterrupt(bool value) override {
        asynch_interrupt = value;
    }
    void budgetOff() override {
        conflict_budget = propagation_budget = 0;
    }

    //TODO: use std::function<int(void*)> as type here
    void setTermCallback(void* state, int (*termCallback)(void*)) {
        this->termCallbackState = state;
        this->termCallback = termCallback;
    }

    void setLearntCallback(void* state, int max_length, void (*learntCallback)(void* state, int* clause)) override { 
        this->learntCallbackState = state;
        this->learntCallbackMaxLength = max_length;
        this->learntCallback = learntCallback;
    }

    void resetCertificate(const char* targetFilename) override {
        this->certificate.reset(targetFilename);
    }

    Certificate certificate;

    // Extra results: (read-only member variable)
    vector<lbool> model; // If problem is satisfiable, this vector contains the model (if any).
    vector<Lit> conflict; // If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.

protected:
    ClauseDatabase& clause_db;
    Trail& trail;
    Propagate& propagator;
    TLearning& conflict_analysis;
    TBranching& branch;

	std::vector<Lit> assumptions; // Current set of assumptions provided to solve by the user.

    // Constants For restarts
    double K;
    double R;
    float sumLBD = 0; // used to compute the global average of LBD. Restarts...
    // Bounded queues for restarts
    bqueue<uint32_t> lbdQueue, trailQueue;

    // used for reduce
    uint_fast64_t curRestart;
    unsigned int nbclausesbeforereduce; // To know when it is time to reduce clause database
    unsigned int incReduceDB;

    // constants for memory reorganization
    bool sort_watches;
    bool sort_variables;

    bool ok; // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!

    bool incremental; // Use incremental SAT Solver

    bool preprocessing_enabled; // do eliminate (via subsumption, asymm, elim)
    std::vector<Var> freezes;

    // inprocessing
    uint64_t lastRestartWithInprocessing;
    uint32_t inprocessingFrequency;

    // Resource contraints and other interrupts
    uint32_t conflict_budget;    // 0 means no budget.
    uint32_t propagation_budget; // 0 means no budget.
    void* termCallbackState;
    int (*termCallback)(void* state);
    bool asynch_interrupt;

    // Learnt callback ipasir
    void* learntCallbackState;
    int learntCallbackMaxLength;
    void (*learntCallback)(void* state, int* clause);

    // Sonification
    SolverSonification sonification;
    ControllerInterface controller;

    void cancelUntil(int level); // Backtrack until a certain level.
    lbool search(); // Search for a given number of conflicts.

    inline bool withinBudget() {
        return !asynch_interrupt && (termCallback == nullptr || 0 == termCallback(termCallbackState))
                && (conflict_budget == 0 || nConflicts() < conflict_budget) && (propagation_budget == 0 || nPropagations() < propagation_budget);
    }
};

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::Solver() : 
    // unsat certificate
    certificate(nullptr),
    // results
    model(), conflict(),
    // Basic Systems
    clause_db(*new TClauseDatabase()),
    trail(*new TAssignment()),
    propagator(*new TPropagate(clause_db, trail)),
	conflict_analysis(*new TLearning(clause_db, trail)),
    branch(*new TBranching(clause_db, trail)),
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
    // conflict state
    ok(true),
    // incremental mode
    incremental(false),
    // preprocessing
    preprocessing_enabled(true),
    freezes(),
    // inprocessing
    lastRestartWithInprocessing(0), inprocessingFrequency(SolverOptions::opt_inprocessing),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr),
    // sonification
    sonification(), controller()
{
controller.run();
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::Solver(TClauseDatabase* _clause_db) : 
    // unsat certificate
    certificate(nullptr),
    // results
    model(), conflict(),
    // Basic Systems
    clause_db(*_clause_db),
    trail(*new TAssignment()),
    propagator(*new TPropagate(clause_db, trail)),
	conflict_analysis(*new TLearning(clause_db, trail)),
    branch(*new TBranching(clause_db, trail)),
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
    // conflict state
    ok(true),
    // incremental mode
    incremental(false),
    // preprocessing
    preprocessing_enabled(true),
    freezes(),
    // inprocessing
    lastRestartWithInprocessing(0), inprocessingFrequency(SolverOptions::opt_inprocessing),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr),
    // sonification
    sonification(), controller()
{
controller.run();
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::Solver(TClauseDatabase& db, TAssignment& as, TPropagate& pr, TLearning& le, TBranching& br) : 
    // unsat certificate
    certificate(nullptr),
    // results
    model(), conflict(),
    // Basic Systems
    clause_db(db),
    trail(as),
    propagator(pr),
	conflict_analysis(le),
    branch(br),
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
    // conflict state
    ok(true),
    // incremental mode
    incremental(false),
    // preprocessing
    preprocessing_enabled(true),
    freezes(),
    // inprocessing
    lastRestartWithInprocessing(0), inprocessingFrequency(SolverOptions::opt_inprocessing),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr),
    // sonification
    sonification(), controller()
{
controller.run();
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::~Solver() {
}

/****************************************************************
 Set the incremental mode
 ****************************************************************/

// This function set the incremental mode to true.
// You can add special code for this mode here.
template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
void Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::setIncrementalMode() {
    incremental = true;
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
bool Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::isIncremental() {
    return incremental;
}

/***
 * Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
 * used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
 ***/
template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
Var Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::newVar() {
    int v = static_cast<int>(nVars());
    clause_db.grow(nVars()+1);
    propagator.init(nVars());
    trail.grow();
    conflict_analysis.grow();
    branch.grow();
    return v;
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
void Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::addClauses(const CNFProblem& dimacs) {
    size_t maxVars = (size_t)dimacs.nVars();
    if (maxVars > this->nVars()) {
        clause_db.grow(maxVars);
        propagator.init(maxVars);
        trail.grow(maxVars);
        conflict_analysis.grow(maxVars);
        branch.grow(maxVars);
    }

    if (sort_variables) {
        branch.initFrom(dimacs);
    }

    for (vector<Lit>* clause : dimacs.getProblem()) {
        if (!addClause(*clause)) {
            certificate.proof();
            return;
        }
    }
    
    if (propagator.propagate() == nullptr) {
        strengthen();
        simplify();
        clause_db.cleanup();
    }
    else {
        ok = false; 
        certificate.proof();
    }
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
template<typename Iterator>
bool Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::addClause(Iterator cbegin, Iterator cend, unsigned int lbd) {
    assert(trail.decisionLevel() == 0);

    std::vector<Lit> copy{cbegin, cend};

    // remove redundant literals
    std::sort(copy.begin(), copy.end());
    copy.erase(std::unique(copy.begin(), copy.end()), copy.end());

    // skip tatological clause
    bool isTautological = copy.end() != std::unique(copy.begin(), copy.end(), [](Lit l1, Lit l2) { return var(l1) == var(l2); });
    if (isTautological) {
        return ok;
    }

    if (copy.size() == 0) {
        return ok = false;
    }
    else if (copy.size() == 1) {
        return ok = trail.newFact(copy.front());
    }
    else {
        Clause* clause = clause_db.createClause(copy, lbd);
        propagator.attachClause(clause);
        return ok;
    }
}

/**************************************************************************************************
 *
 *  simplify : [void]  ->  [bool]
 *
 *  Description:
 *    Simplify the clause database according to the current top-level assignment. Currently, the only
 *    thing done here is the removal of satisfied clauses, but more things can be put here.
 **************************************************************************************************/
template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
void Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::simplify() {
    assert(trail.decisionLevel() == 0);
    assert(propagator.propagate() == nullptr);

    for (const Clause* clause : clause_db.clauses) if (!clause->isDeleted()) {
        if (trail.satisfied(*clause)) {
            certificate.removed(clause->begin(), clause->end());
            propagator.detachClause(clause);
            clause_db.removeClause((Clause*)clause);
        }
    }
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
void Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::strengthen() {
    assert(trail.decisionLevel() == 0);
    assert(propagator.propagate() == nullptr);

    for (const Clause* clause : clause_db.clauses) if (!clause->isDeleted()) {
        vector<Lit> copy(clause->begin(), clause->end());

        for (Lit lit : copy) {        
            if (trail.value(lit) == l_False) {
                if (clause->size() == copy.size()) { // first match
                    propagator.detachClause(clause);
                }
                clause_db.strengthenClause((Clause*)clause, lit);
            }
        }

        if (clause->size() < copy.size()) {
            assert(clause->size() > 0);

            certificate.added(clause->begin(), clause->end());
            certificate.removed(copy.begin(), copy.end());

            if (clause->size() == 1) {
                trail.newFact(clause->first());
            }
            else {
                propagator.attachClause((Clause*)clause);
            }
        }
    }
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
template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
lbool Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::search() {
    assert(ok);
    
    bool blocked = false;
    Statistics::getInstance().solverRestartInc();
    sonification.restart();
    for (;;) {
        sonification.decisionLevel(trail.decisionLevel(), SolverOptions::opt_sonification_delay);

        Clause* confl = propagator.propagate();
        
        sonification.assignmentLevel(static_cast<int>(trail.size()));
        
        if (confl != nullptr) { // CONFLICT
            sonification.conflictLevel(trail.decisionLevel());

            if (trail.decisionLevel() == 0) {
                return l_False;
            }
            
            trailQueue.push(trail.size());
            
            // BLOCK RESTART (CP 2012 paper)
            if (nConflicts() > 10000 && lbdQueue.isvalid() && trail.size() > R * trailQueue.getavg()) {
                lbdQueue.fastclear();
                Statistics::getInstance().solverStopsRestartsInc();
                if (!blocked) {
                    Statistics::getInstance().solverLastBlockAtRestartSave();
                    Statistics::getInstance().solverStopsRestartsSameInc();
                    blocked = true;
                }
            }
            
            conflict_analysis.analyze(confl);
            AnalysisResult& conflictInfo = clause_db.getConflictResult();
            if (incremental) {
            	// sort selectors to back (but keep asserting literal upfront: begin+1)
            	std::sort(conflictInfo.learnt_clause.begin()+1, conflictInfo.learnt_clause.end(), [this](Lit lit1, Lit lit2) { return (!isSelector(lit1) && isSelector(lit2)) || lit1 < lit2; });
            }
            
            if (learntCallback != nullptr && (int)conflictInfo.learnt_clause.size() <= learntCallbackMaxLength) {
                vector<int> clause;
                clause.reserve(conflictInfo.learnt_clause.size() + 1);
                for (Lit lit : conflictInfo.learnt_clause) {
                    clause.push_back((var(lit)+1)*(sign(lit)?-1:1));
                }
                clause.push_back(0);
                learntCallback(learntCallbackState, clause.data());
            }

            sonification.learntSize(static_cast<int>(conflictInfo.learnt_clause.size()));

            if (!isSelector(conflictInfo.learnt_clause.back())) {
                certificate.added(conflictInfo.learnt_clause.begin(), conflictInfo.learnt_clause.end());
            }

            trail.reduceLBDs(conflictInfo.involved_clauses);
            clause_db.bumpActivities(conflictInfo.involved_clauses); 

            branch.notify_conflict();

            lbdQueue.push(conflictInfo.lbd);
            sumLBD += conflictInfo.lbd;

            if (conflictInfo.learnt_clause.size() == 1) {
                trail.cancelUntil(0);
                trail.uncheckedEnqueue(conflictInfo.learnt_clause[0]);
            }
            else {
                unsigned int backtrack_level = trail.level(var(conflictInfo.learnt_clause[1]));
                for (unsigned int i = 2; i < conflictInfo.learnt_clause.size(); i++) {
                    unsigned int level = trail.level(var(conflictInfo.learnt_clause[i]));
                    if (level > backtrack_level) {
                        backtrack_level = level;
                        std::swap(conflictInfo.learnt_clause[1], conflictInfo.learnt_clause[i]);
                    }
                }

                Clause* clause = clause_db.createClause(conflictInfo.learnt_clause, conflictInfo.lbd);

                trail.cancelUntil(backtrack_level);
                trail.uncheckedEnqueue(clause->first(), clause);

                propagator.attachClause(clause);
            }
            clause_db.decayActivity();
            branch.notify_backtracked();
        }
        else {
            // Our dynamic restart, see the SAT09 competition compagnion paper
            if ((lbdQueue.isvalid() && ((lbdQueue.getavg() * K) > (sumLBD / nConflicts())))) {
                lbdQueue.fastclear();
                
                trail.cancelUntil(0);
                branch.notify_restarted();
                
                // Perform clause database reduction and simplifications
                if (nConflicts() >= (curRestart * nbclausesbeforereduce)) {
                    curRestart = (nConflicts() / nbclausesbeforereduce) + 1;

                    if (inprocessingFrequency > 0 && lastRestartWithInprocessing + inprocessingFrequency <= curRestart) {
                        lastRestartWithInprocessing = curRestart;
                        if (!eliminate(false, false)) {
                            return l_False;
                        }
                    }
                    
                    // clause database simplification
                    strengthen();
                    simplify();
                    clause_db.cleanup();
                
                    // clause database reduction
                    propagator.detachAll();
                    clause_db.reduce();
                    for (const Clause* clause : clause_db) {
                        if (clause->isDeleted()) {
                            certificate.removed(clause->begin(), clause->end());
                        }
                    }
                    clause_db.cleanup();
                    clause_db.defrag();
                    propagator.attachAll(clause_db.clauses);

                    if (sort_watches) {
                        propagator.sortWatchers();
                    }

                    nbclausesbeforereduce += incReduceDB;
                }
                
                return l_Undef;
            }
            
            Lit next = lit_Undef;
            while (trail.decisionLevel() < assumptions.size()) {
                // Perform user provided assumption:
                Lit p = assumptions[trail.decisionLevel()];
                if (trail.value(p) == l_True) {
                    // Dummy decision level:
                    trail.newDecisionLevel();
                } else if (trail.value(p) == l_False) {
                    conflict_analysis.analyzeFinal(~p);
                    AnalysisResult& result = clause_db.getConflictResult();
                    conflict.insert(conflict.end(), result.learnt_clause.begin(), result.learnt_clause.end());
                    return l_False;
                } else {
                    next = p;
                    break;
                }
            }
            
            if (next == lit_Undef) {
                // New variable decision:
                Statistics::getInstance().solverDecisionsInc();
                next = branch.pickBranchLit();
                if (next == lit_Undef) {
                    // Model found:
                    return l_True;
                }
            }
            
            // Increase decision level and enqueue 'next'
            trail.newDecisionLevel();
            trail.uncheckedEnqueue(next);
        }
    }
    return l_Undef; // not reached
}

// NOTE: assumptions passed in member-variable 'assumptions'.
// Parameters are useless in core but useful for SimpSolver....
template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
lbool Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::solve() {
    Statistics::getInstance().runtimeStart("Solver");

    sonification.start(static_cast<int>(nVars()), static_cast<int>(nClauses()));
    
    model.clear();
    conflict.clear();

    lbool status = l_Undef;
    if (isInConflictingState()) {
        status = l_False;
    }
    
    // Search:
    while (status == l_Undef && withinBudget()) {
        status = search();
    }
    
    if (status == l_False) {
        if (!incremental) {
            certificate.proof();
        }
        else {
            // check if selectors are used in final conflict
            conflict_analysis.analyzeFinal(trail[trail.size()-1]);
            AnalysisResult& result = clause_db.getConflictResult();
            auto pos = find_if(result.learnt_clause.begin(), result.learnt_clause.end(), [this] (Lit lit) { return isSelector(var(lit)); } );
            if (pos == result.learnt_clause.end()) {
                certificate.proof();
            }
        }
        
        if (conflict.size() == 0) {
            ok = false;
        }

        sonification.stop(1);
    }
    else if (status == l_True) {
        model.insert(model.end(), trail.assigns.begin(), trail.assigns.end());

        sonification.stop(0);
    }
    else {
        sonification.stop(-1);
    }
    
    trail.cancelUntil(0);

    Statistics::getInstance().runtimeStop("Solver");
    return status;
}

}

#endif
