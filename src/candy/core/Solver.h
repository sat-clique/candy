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

#ifndef CANDY_SOLVER_H
#define CANDY_SOLVER_H

#include <vector>
#include <unordered_map>
#include <math.h>
#include <string>
#include <type_traits>
#include <memory>
#include <limits>

#include "candy/frontend/CLIOptions.h"

#include "candy/mtl/Stamp.h"
#include "candy/mtl/Heap.h"
#include "candy/mtl/BoundedQueue.h"

#include "candy/randomsimulation/Conjectures.h"
#include "candy/rsar/Refinement.h"

#include "candy/core/learning/ConflictAnalysis.h"
#include "candy/core/propagate/Propagate.h"
#include "candy/core/branching/VSIDS.h"
#include "candy/core/simplification/Subsumption.h"
#include "candy/core/simplification/VariableElimination.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/Clause.h"

#include "candy/core/CandySolverInterface.h"
#include "candy/core/Statistics.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/Certificate.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/Trail.h"

#include "candy/utils/System.h"
#include "candy/utils/Attributes.h"
#include "candy/utils/CheckedCast.h"

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
template<class TClauses = ClauseDatabase, class TAssignment = Trail, class TPropagate = Propagate, class TLearning = ConflictAnalysis, class TBranching = VSIDS> 
class Solver : public CandySolverInterface {
    static_assert(std::is_class<TBranching>::value, "TBranching must be a class");
    //static_assert(std::is_constructible<TBranching>::value, "TBranching must have a constructor without arguments");
    //static_assert(std::is_move_assignable<TBranching>::value, "TBranching must be move-assignable");
    //static_assert(std::is_move_constructible<TBranching>::value, "TBranching must be move-constructible");

public:
    Solver();
    Solver(TClauses db, TAssignment& as);
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
        for (const Clause* clause : clause_db) {
            clause->printDIMACS();
        }
    }

    vector<Lit>& getConflict() override {
        return conflict;
    }

    // Solving:
    void unit_resolution() override; // remove satisfied clauses and remove false literals from clauses 
    void eliminate() override; // Perform variable elimination based simplification. 

    BranchingDiversificationInterface* accessBranchingInterface() override {
        return &branch;
    }
    
    void enablePreprocessing() override {
        preprocessing_enabled = true;
    }

    void disablePreprocessing() override {
        preprocessing_enabled = false;
    }

    bool isEliminated(Var v) const override { 
        return elimination.isEliminated(v);
    }

    void setFrozen(Var v, bool freeze) override  {
        if (freeze) {
            freezes.push_back(v);
        } else {
            freezes.erase(std::remove(freezes.begin(), freezes.end(), v), freezes.end());
        }
    }

    lbool solve() override;

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
        return clause_db.size();
    }
    size_t nVars() const override {
        return trail.vardata.size();
    }
    size_t nConflicts() const override {
        return clause_db.result.nConflicts;
    }
    size_t nPropagations() const override {
        return propagator.nPropagations;
    }

    bool isSelector(Var v) {
        return false;
    }

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

    Certificate certificate;

    // Extra results: (read-only member variable)
    vector<lbool> model; // If problem is satisfiable, this vector contains the model (if any).
    vector<Lit> conflict; // If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.

protected:
    TClauses clause_db;
    Trail& trail;
    TPropagate propagator;
    TLearning conflict_analysis;
    TBranching branch;

    Subsumption<TPropagate> subsumption;
    VariableElimination<TPropagate> elimination;

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

    bool ok; // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!

    bool preprocessing_enabled; // do eliminate (via subsumption, asymm, elim)
    double simplification_threshold_factor = 0.1;
    std::vector<Var> freezes;
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

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::Solver() : 
    // unsat certificate
    certificate(SolverOptions::opt_certified_file),
    // results
    model(), conflict(),
    // Basic Systems
    clause_db(),
    trail(*new TAssignment()),
    propagator(clause_db, trail),
	conflict_analysis(clause_db, trail),
    branch(clause_db, trail),
    // simplification
    subsumption(clause_db, trail, propagator, certificate),
    elimination(clause_db, trail, propagator, certificate), 
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
    // conflict state
    ok(true),
    // pre- and inprocessing
    preprocessing_enabled(SolverOptions::opt_preprocessing),
    simplification_threshold_factor(SolverOptions::opt_simplification_threshold_factor),
    freezes(),
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

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::Solver(TClauses db, TAssignment& as) : 
    // unsat certificate
    certificate(SolverOptions::opt_certified_file),
    // results
    model(), conflict(),
    // Basic Systems
    clause_db(std::move(db)),
    trail(as),
    propagator(clause_db, trail),
	conflict_analysis(clause_db, trail),
    branch(clause_db, trail),
    // simplification
    subsumption(clause_db, trail, propagator, certificate),
    elimination(clause_db, trail, propagator, certificate), 
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
    // conflict state
    ok(true),
    // preprocessing
    preprocessing_enabled(SolverOptions::opt_preprocessing),
    simplification_threshold_factor(SolverOptions::opt_simplification_threshold_factor),
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

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::~Solver() {
}

/***
 * Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
 * used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
 ***/
template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
Var Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::newVar() {
    int v = static_cast<int>(nVars());
    clause_db.grow(nVars()+1);
    propagator.init(nVars());
    trail.grow();
    conflict_analysis.grow();
    branch.grow();
    elimination.grow(nVars());
    return v;
}

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
void Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::addClauses(const CNFProblem& dimacs) {
    size_t maxVars = (size_t)dimacs.nVars();
    if (maxVars > this->nVars()) {
        clause_db.grow(maxVars);
        propagator.init(maxVars);
        trail.grow(maxVars);
        conflict_analysis.grow(maxVars);
        branch.grow(maxVars);
        elimination.grow(maxVars);
    }

    branch.init(dimacs);

    for (vector<Lit>* clause : dimacs.getProblem()) {
        if (!addClause(*clause)) {
            certificate.proof();
            return;
        }
    }
    
    if (propagator.propagate() == nullptr) {
        unit_resolution();
    }
    else {
        ok = false; 
        certificate.proof();
    }
}

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
template<typename Iterator>
bool Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::addClause(Iterator cbegin, Iterator cend, unsigned int lbd) {
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
        Clause* clause = clause_db.createClause(copy.begin(), copy.end(), lbd);
        propagator.attachClause(clause);
        return ok;
    }
}

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
void Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::unit_resolution() {
    assert(trail.decisionLevel() == 0);
    assert(propagator.propagate() == nullptr);

    vector<Lit> literals;

    for (size_t i = 0, size = clause_db.size(); i < size; i++) { // use index instead of iterator, as new clauses are created here
        const Clause* clause = clause_db[i];

        if (clause->isDeleted()) continue; 

        literals.clear();

        for (Lit lit : *clause) {
            lbool value = trail.value(lit);
            if (value == l_Undef) {
                literals.push_back(lit);
            }
            else if (value == l_True) {
                literals.clear();
                break;
            }
        }
        
        if (literals.size() < clause->size()) {
            if (literals.size() > 0) {
                if (literals.size() == 1) {
                    trail.newFact(literals.front());
                }
                else {
                    Clause* new_clause = clause_db.createClause(literals.begin(), literals.end(), clause->getLBD());
                    propagator.attachClause(new_clause);
                }
                certificate.added(literals.begin(), literals.end());
            }
            certificate.removed(clause->begin(), clause->end());
            propagator.detachClause(clause);
            clause_db.removeClause((Clause*)clause);
        }
    }
}

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
void Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::eliminate() {
    // freeze assumptions and other externally set frozen variables
    for (Lit lit : assumptions) {
        elimination.lock(var(lit));
    }
    for (Var var : freezes) {
        elimination.lock(var);
    }

    clause_db.initOccurrenceTracking();

    unsigned int num = 1;
    unsigned int max = 0;
    while (num > max * simplification_threshold_factor) {
        ok = subsumption.subsume();
        if (isInConflictingState() || asynch_interrupt) break;

        ok = elimination.eliminate();
        if (isInConflictingState() || asynch_interrupt) break;
        
        num = subsumption.nStrengthened + subsumption.nSubsumed + elimination.nEliminated + elimination.nStrengthened;
        max = std::max(num, max);
        Statistics::getInstance().printSimplificationStats();
    } 

    for (unsigned int v = 0; v < this->nVars(); v++) {
        if (elimination.isEliminated(v)) {
            branch.setDecisionVar(v, false);
        }
    }

    branch.notify_restarted(); // former rebuildOrderHeap

    clause_db.stopOccurrenceTracking();

    if (max > nClauses() * simplification_threshold_factor) {
        propagator.detachAll();
        clause_db.defrag();
        propagator.attachAll();
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
template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
lbool Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::search() {
    assert(ok);
    
    bool blocked = false;
    Statistics::getInstance().solverRestartInc();
    sonification.restart();
    for (;;) {
        sonification.decisionLevel(trail.decisionLevel(), SolverOptions::opt_sonification_delay);

        Clause* confl = (Clause*)propagator.propagate();
        
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
            
            conflict_analysis.handle_conflict(confl);
            
            if (learntCallback != nullptr && (int)clause_db.result.learnt_clause.size() <= learntCallbackMaxLength) {
                vector<int> clause;
                clause.reserve(clause_db.result.learnt_clause.size() + 1);
                for (Lit lit : clause_db.result.learnt_clause) {
                    clause.push_back((var(lit)+1)*(sign(lit)?-1:1));
                }
                clause.push_back(0);
                learntCallback(learntCallbackState, clause.data());
            }

            sonification.learntSize(static_cast<int>(clause_db.result.learnt_clause.size()));
            
            certificate.added(clause_db.result.learnt_clause.begin(), clause_db.result.learnt_clause.end());

            branch.process_conflict();

            lbdQueue.push(clause_db.result.lbd);
            sumLBD += clause_db.result.lbd;

            trail.cancelUntil(clause_db.result.backtrack_level);

            assert(trail.value(clause_db.result.learnt_clause[0]) == l_Undef);
            
            if (clause_db.result.learnt_clause.size() == 1) {
                trail.uncheckedEnqueue(clause_db.result.learnt_clause[0]);
            }
            else {
                Clause* clause = clause_db.createClause(clause_db.result.learnt_clause.begin(), clause_db.result.learnt_clause.end(), clause_db.result.lbd);
                trail.uncheckedEnqueue(clause->first(), clause);
                propagator.attachClause(clause);
            }
        }
        else {
            // Our dynamic restart, see the SAT09 competition compagnion paper
            if (nConflicts() > 0 && (lbdQueue.isvalid() && ((lbdQueue.getavg() * K) > (sumLBD / nConflicts())))) {
                lbdQueue.fastclear();
                
                trail.cancelUntil(0);
                branch.notify_restarted();
                
                // Perform clause database reduction and simplifications
                if (nConflicts() >= (curRestart * nbclausesbeforereduce)) {
                    curRestart = (nConflicts() / nbclausesbeforereduce) + 1;

                    if (inprocessingFrequency > 0 && lastRestartWithInprocessing + inprocessingFrequency <= curRestart) {
                        lastRestartWithInprocessing = curRestart;
                        eliminate();
                        if (isInConflictingState()) {
                            return l_False;
                        }
                    }
                    
                    unit_resolution();
                    
                    propagator.detachAll();
                    std::vector<Clause*> reduced = clause_db.reduce();
                    for (const Clause* clause : reduced) {
                        certificate.removed(clause->begin(), clause->end());
                    }
                    clause_db.defrag();
                    propagator.attachAll();

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
                    trail.newDecisionLevel(); // Dummy decision level
                } else if (trail.value(p) == l_False) {
                    conflict = conflict_analysis.analyzeFinal(~p);
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

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
lbool Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::solve() {
    if (isInConflictingState()) return l_False;
    
    model.clear();
    conflict.clear();

    if (this->preprocessing_enabled) {
        Statistics::getInstance().runtimeStart("Preprocessing");
        eliminate();
        Statistics::getInstance().printSimplificationStats();
        Statistics::getInstance().runtimeStop("Preprocessing");
    }
    
    if (isInConflictingState()) return l_False;

    Statistics::getInstance().runtimeStart("Solving");

    sonification.start(static_cast<int>(nVars()), static_cast<int>(nClauses()));

    lbool status = l_Undef;
    while (status == l_Undef && withinBudget()) {
        status = search();
    }
    
    if (status == l_False) {
        if (conflict.empty()) {
            ok = false;
            certificate.proof();
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

    Statistics::getInstance().runtimeStop("Solving");
        
    if (status == l_True) {
        elimination.extendModel(this->model);
    }
    
    return status;
}

}

#endif
