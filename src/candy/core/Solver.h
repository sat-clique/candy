/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Candy sources are based on Glucose which is based on MiniSat (see former copyrights below). 
Permissions and copyrights of Candy are exactly the same as Glucose and Minisat (see below).


--------------- Former Glucose Copyrights

 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
                                CRIL - Univ. Artois, France
                                LRI  - Univ. Paris Sud, France (2009-2013)
                                Labri - Univ. Bordeaux, France

Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it 
is based on. (see below).


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
*************************************************************************************************/

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

#include "candy/randomsimulation/Conjectures.h"
#include "candy/rsar/Refinement.h"

#include "candy/core/learning/ConflictAnalysis.h"
#include "candy/core/propagate/Propagate.h"
#include "candy/core/branching/VSIDS.h"
#include "candy/core/simplification/Subsumption.h"
#include "candy/core/simplification/SubsumptionClauseDatabase.h"
#include "candy/core/simplification/VariableElimination.h"
#include "candy/core/simplification/AsymmetricVariableReduction.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/Clause.h"

#include "candy/core/CandySolverInterface.h"
#include "candy/core/Statistics.h"
#include "candy/core/Logging.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/Trail.h"
#include "candy/core/CandySolverResult.h"
#include "candy/core/Restart.h"

#include "candy/utils/Attributes.h"
#include "candy/utils/CheckedCast.h"

namespace Candy {

template<class TClauses = ClauseDatabase, class TAssignment = Trail, class TPropagate = Propagate, class TLearning = ConflictAnalysis, class TBranching = VSIDS> 
class Solver : public CandySolverInterface {
public:
    Solver();
    ~Solver();

    void init(const CNFProblem& problem, ClauseAllocator* allocator = nullptr) override;

    ClauseAllocator* setupGlobalAllocator() override {
        return clause_db.createGlobalClauseAllocator();
    }

    CandySolverResult& getCandySolverResult() override { 
        return result;
    }

    ClauseDatabase& getClauseDatabase() override {
        return clause_db;
    }

    Trail& getAssignment() override {
        return trail;
    }

    Statistics& getStatistics() override {
        return statistics; 
    }

    BranchingDiversificationInterface* getBranchingUnit() override {
        return &branch;
    }

    lbool solve() override;

    void setAssumptions(const std::vector<Lit>& assumptions) override {
        this->assumptions.clear();
        this->assumptions.insert(this->assumptions.end(), assumptions.begin(), assumptions.end());
        for (Lit lit : assumptions) {
            elimination.lock(lit.var());
        }
    }

    //TODO: use std::function<int(void*)> as type here
    void setTermCallback(void* state, int (*termCallback)(void* state)) override {
        this->termCallbackState = state;
        this->termCallback = termCallback;
    }

    void setLearntCallback(void* state, int max_length, void (*learntCallback)(void* state, int* clause)) override { 
        this->learntCallbackState = state;
        this->learntCallbackMaxLength = max_length;
        this->learntCallback = learntCallback;
    }

protected:
    TClauses clause_db;
    TAssignment trail;
    TPropagate propagator;
    TLearning conflict_analysis;
    TBranching branch;

    Restart restart;

    SubsumptionClauseDatabase augmented_database;
    Subsumption subsumption;
    VariableElimination elimination;
    AsymmetricVariableReduction<TPropagate> reduction;

    Statistics statistics;
    Logging logging;

    CandySolverResult result;

	std::vector<Lit> assumptions; // Current set of assumptions provided to solve by the user.

    // reduce-db
    unsigned int nbclausesbeforereduce; // To know when it is time to reduce clause database
    unsigned int incReduceDB;

    bool ok; // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!

    bool preprocessing_enabled; // do eliminate (via subsumption, asymm, elim)
    Stamp<Var> freezes;

    unsigned int lastRestartWithInprocessing;
    unsigned int inprocessingFrequency;

    // Interruption callback
    void* termCallbackState;
    int (*termCallback)(void* state);

    // Learnt callback ipasir
    void* learntCallbackState;
    int learntCallbackMaxLength;
    void (*learntCallback)(void* state, int* clause);

    lbool search(); 

private:
    // true means solver is in a conflicting state
    bool isInConflictingState() const {
        return !ok;
    }

    void propagateAndMaterializeUnitClauses() {
        assert(trail.decisionLevel() == 0);
        std::vector<Clause*> facts = clause_db.getUnitClauses();
        for (Clause* clause : facts) {
            this->ok &= trail.fact(clause->first());
        }
        if (!isInConflictingState()) {
            std::array<Lit, 1> unit;
            unsigned int pos = trail.size();
            this->ok &= (propagator.propagate() == nullptr);
            if (!isInConflictingState()) {
                for (auto it = trail.begin() + pos; it != trail.end(); it++) {
                    assert(trail.reason(it->var()) != nullptr);
                    unit[0] = *it;
                    Clause* new_clause = clause_db.createClause(unit.begin(), unit.end());
                    trail.fact(new_clause->first());
                }
            }
        }
    }

    void processClauseDatabase() {
        assert(trail.decisionLevel() == 0);

        augmented_database.initialize();

        unsigned int num = 1;
        unsigned int max = 0;
        double simplification_threshold_factor = 0.1;
        while (num > max * simplification_threshold_factor && termCallback(termCallbackState) == 0) {
            propagateAndMaterializeUnitClauses();
            if (isInConflictingState()) break;

            ok &= subsumption.subsume();

            if (subsumption.nTouched() > 0) {
                augmented_database.cleanup();
                std::cout << "c " << std::this_thread::get_id() << ": Subsumption subsumued " << subsumption.nSubsumed << " and strengthened " << subsumption.nStrengthened << " clauses" << std::endl;
            }

            propagateAndMaterializeUnitClauses();
            if (isInConflictingState()) break;

            ok &= reduction.reduce();

            if (reduction.nTouched() > 0) {
                augmented_database.cleanup();
                std::cout << "c " << std::this_thread::get_id() << ": Reduction strengthened " << reduction.nTouched() << " clauses" << std::endl;
            }

            propagateAndMaterializeUnitClauses();
            if (isInConflictingState()) break;

            ok &= elimination.eliminate();

            if (elimination.nTouched() > 0) {
                augmented_database.cleanup();
                std::cout << "c " << std::this_thread::get_id() << ": Eliminiated " << elimination.nTouched() << " variables" << std::endl;
                for (unsigned int v = 0; v < statistics.nVars(); v++) {
                    if (elimination.isEliminated(v)) {
                        branch.setDecisionVar(v, false);
                    }
                }
            }

            num = subsumption.nTouched() + elimination.nTouched() + reduction.nTouched();
            max = std::max(num, max);
        } 

        augmented_database.clear();
    }

};

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::Solver() : 
    // Basic Systems
    clause_db(),
    trail(),
    propagator(clause_db, trail),
	conflict_analysis(clause_db, trail),
    branch(clause_db, trail),
    restart(clause_db, trail),
    // simplification
    augmented_database(clause_db),
    subsumption(augmented_database),
    elimination(augmented_database, trail), 
    reduction(augmented_database, trail, propagator), 
    // stats
    statistics(*this), 
    logging(*this),
    // result
    result(),
    // assumptions 
    assumptions(),
    // reduce db heuristic control
    nbclausesbeforereduce(ClauseDatabaseOptions::opt_first_reduce_db),
    incReduceDB(ClauseDatabaseOptions::opt_inc_reduce_db),
    // conflict state
    ok(true),
    // pre- and inprocessing
    preprocessing_enabled(SolverOptions::opt_preprocessing),
    freezes(),
    lastRestartWithInprocessing(0), inprocessingFrequency(SolverOptions::opt_inprocessing), 
    // interruption callback
    termCallbackState(nullptr), termCallback([](void*) -> int { return 0; }),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr)
{ }

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::~Solver() {
}

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
void Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::init(const CNFProblem& problem, ClauseAllocator* allocator ) {
    assert(trail.decisionLevel() == 0);

    statistics.runtimeStart("Wallclock");

    if (problem.nVars() > statistics.nVars()) {
        clause_db.grow(problem.nVars());
        propagator.init(problem.nVars());
        trail.grow(problem.nVars());
        conflict_analysis.grow(problem.nVars());
        branch.grow(problem.nVars());
        elimination.grow(problem.nVars());
        reduction.grow(problem.nVars());
        augmented_database.grow(problem.nVars());
    }

    if (allocator == nullptr) {
        std::cout << "c importing " << problem.nClauses() << " clauses from dimacs" << std::endl;
        for (Cl* import : problem) {
            Clause* clause = clause_db.createClause(import->begin(), import->end());
            if (clause->size() > 2) {
                propagator.attachClause(clause);
            } 
            else if (clause->size() == 0) {
                this->ok = false;
                return;
            }
        }
    } 
    else {
        std::cout << "c importing clauses from global allocator" << std::endl;
        clause_db.setGlobalClauseAllocator(allocator);
        for (Clause* clause : clause_db) {
            if (clause->size() > 2) {
                propagator.attachClause(clause);
            } 
            else if (clause->size() == 0) {
                this->ok = false;
                return;
            }
        }
    }

    branch.init(problem);

    statistics.runtimeStop("Wallclock");
}


template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
lbool Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::search() {
    assert(ok);
    
    statistics.solverRestartInc();
    logging.logRestart();
    for (;;) {
        Clause* confl = (Clause*)propagator.propagate();
        logging.logDecision();
        if (confl != nullptr) { // CONFLICT
            logging.logConflict();

            if (trail.decisionLevel() == 0) {
                std::cout << "c Conflict found by propagation at level 0" << std::endl;
                return l_False;
            }
            
            conflict_analysis.handle_conflict(confl);

            branch.process_conflict();
            restart.process_conflict();

            Clause* clause = clause_db.createClause(clause_db.result.learnt_clause.begin(), clause_db.result.learnt_clause.end(), clause_db.result.lbd);
            if (clause->size() > 2) {
                propagator.attachClause(clause);
            }

            trail.backtrack(clause_db.result.backtrack_level);
            trail.propagate(clause->first(), clause);
            
            if (learntCallback != nullptr && clause->size() <= learntCallbackMaxLength) {
                std::vector<int> to_send;
                to_send.reserve(clause->size() + 1);
                for (Lit lit : *clause) {
                    to_send.push_back((lit.var()+1)*(lit.sign()?-1:1));
                }
                to_send.push_back(0);
                learntCallback(learntCallbackState, to_send.data());
            }

            logging.logLearntClause(clause);
        }
        else {
            if (restart.trigger_restart()) {
                return l_Undef;
            }
            
            Lit next = lit_Undef;
            while (trail.decisionLevel() < assumptions.size()) {
                // Perform user provided assumption:
                Lit p = assumptions[trail.decisionLevel()];
                if (trail.value(p) == l_True) {
                    trail.newDecisionLevel(); // Dummy decision level
                } 
                else if (trail.value(p) == l_False) {
                    std::cout << "c Conflict found during assumption propagation" << std::endl;
                    result.setConflict(conflict_analysis.analyzeFinal(~p));
                    return l_False;
                } 
                else {
                    next = p;
                    break;
                }
            }
            
            if (next == lit_Undef) {
                // New variable decision:
                next = branch.pickBranchLit();
                if (next == lit_Undef) { // Model found
                    return l_True;
                }
            }
            
            // Increase decision level and enqueue 'next'
            trail.newDecisionLevel();
            trail.decide(next);
        }
    }
    return l_Undef; // not reached
}

template<class TClauses, class TAssignment, class TPropagate, class TLearning, class TBranching>
lbool Solver<TClauses, TAssignment, TPropagate, TLearning, TBranching>::solve() {
    statistics.runtimeStart("Wallclock");
    logging.logStart();
    
    result.clear();

    if (this->preprocessing_enabled) {
        trail.reset();
        processClauseDatabase();
        // in case of global allocation synchronization now is mandatory (due to DLD: delayed lazy deletion)
        clause_db.reorganize(); 
        propagator.reset();
    }

    lbool status = isInConflictingState() ? l_False : l_Undef;

    while (status == l_Undef && termCallback(termCallbackState) == 0) {
        trail.reset();
        branch.reset();

        if (statistics.nReduceCalls() * nbclausesbeforereduce <= statistics.nConflicts()) {
            nbclausesbeforereduce += incReduceDB;

            if (inprocessingFrequency > 0 && lastRestartWithInprocessing + inprocessingFrequency <= statistics.nReduceCalls()) { 
                std::cout << "c " << std::this_thread::get_id() << ": Inprocessing " << statistics.nReduceCalls() << " (Restart " << statistics.nRestarts() << ", Database size " << clause_db.size() << ")." << std::endl;
                lastRestartWithInprocessing = statistics.nReduceCalls();
                processClauseDatabase();
            }
            else {
                std::cout << "c " << std::this_thread::get_id() << ": Reduction " << statistics.nReduceCalls() << " (Restart " << statistics.nRestarts() << ", Database size " << clause_db.size() << ")." << std::endl;
                clause_db.reduce();
            }
            // in case of global allocation synchronization now is mandatory (due to DLD: delayed lazy deletion)
            clause_db.reorganize();
            std::cout << "c " << std::this_thread::get_id() << ": Database size is now " << clause_db.size() << std::endl;
            propagator.reset();
        }

        // multi-threaded unit-clauses fast-track
        propagateAndMaterializeUnitClauses();

        if (isInConflictingState()) {
            status = l_False;
        } 
        else {
            status = search();
        }
    }

    if (status == l_Undef) {
        std::cout << "c " << std::this_thread::get_id() << ": Interrupted" << std::endl;
    }
    else {
        std::cout << "c " << std::this_thread::get_id() << ": Found solution" << std::endl;
    }
    
    result.setStatus(status);

    if (status == l_False) {
        if (result.getConflict().empty()) {
            clause_db.emptyClause();
        }
    }
    else if (status == l_True) {
        result.setModel(trail);
        elimination.extendModel(result.getModelValues());
    }
    
    trail.backtrack(0);
    
    logging.logResult(status);
    statistics.runtimeStop("Wallclock"); 
    return status;
}

}

#endif
