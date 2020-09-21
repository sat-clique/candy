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

#include "candy/utils/CLIOptions.h"

#include "candy/mtl/Stamp.h"

#include "candy/systems/learning/ConflictAnalysis.h"
#include "candy/systems/propagate/Propagate.h"
#include "candy/systems/branching/VSIDS.h"
#include "candy/systems/Restart.h"
#include "candy/systems/ReduceDB.h"

#include "candy/simplification/Subsumption.h"
#include "candy/simplification/OccurenceList.h"
#include "candy/simplification/VariableElimination.h"
#include "candy/simplification/AsymmetricVariableReduction.h"

#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/Clause.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/Trail.h"
#include "candy/core/CandySolverResult.h"

#include "candy/utils/Memory.h"
#include "candy/utils/Runtime.h"

namespace Candy {

template<class TPropagate = Propagate, class TLearning = ConflictAnalysis, class TBranching = VSIDS> 
class Solver : public CandySolverInterface {
public:
    Solver();
    ~Solver();

    void clear() override;
    void init(const CNFProblem& problem, ClauseAllocator* allocator = nullptr, bool lemma = true) override;

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

    BranchingDiversificationInterface* getBranchingUnit() override {
        return &branch;
    }

    unsigned int nVars() const override {
        return trail.vardata.size();
    }

    unsigned int nClauses() const override {
        return clause_db.size();
    }

    unsigned int nConflicts() const override {
        return clause_db.result.nConflicts;
    }

    unsigned int nPropagations() const {
        return trail.nPropagations;
    }

    unsigned int nDecisions() const {
        return trail.nDecisions;
    }

    unsigned int nRestarts() const {
        return restart.nRestarts();
    }

    lbool solve() override;

    void printStats();

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
    ClauseDatabase clause_db;
    Trail trail;

    TPropagate propagator;
    TLearning conflict_analysis;
    TBranching branch;

    Restart restart;
    ReduceDB reduce;

    unsigned int verbosity;

    CandySolverResult result;

    bool preprocessing_enabled; // do eliminate (via subsumption, asymm, elim)

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
    // important in parallel scenario
    void propagateAndMaterializeUnitClauses() {
        assert(trail.decisionLevel() == 0);
        std::vector<Clause*> facts = clause_db.getUnitClauses();
        for (Clause* clause : facts) {
            assert(clause->size() == 1);
            if (!trail.fact(clause->first())) clause_db.emptyClause();
        }
        if (!clause_db.hasEmptyClause()) {
            std::array<Lit, 1> unit;
            unsigned int pos = trail.size();            
            if (propagator.propagate() != nullptr) { clause_db.emptyClause(); }
            else {
                for (auto it = trail.begin() + pos; it != trail.end(); it++) {
                    assert(trail.reason(it->var()) != nullptr);
                    unit[0] = *it;
                    Clause* new_clause = clause_db.createClause(unit.begin(), unit.end());
                    trail.fact(new_clause->first());
                }
            }
        }
    }

    // pre- and inprocessing
    void processClauseDatabase() {
        assert(trail.decisionLevel() == 0);

        OccurenceList occurence_list { clause_db };
        Subsumption subsumption { occurence_list };
        VariableElimination elimination { occurence_list, clause_db, trail };
        AsymmetricVariableReduction<TPropagate> reduction { occurence_list, trail, propagator };

        unsigned int num = 1;
        unsigned int max = 0;
        double simplification_threshold_factor = 0.1;
        while (num > max * simplification_threshold_factor && termCallback(termCallbackState) == 0) {
            // propagateAndMaterializeUnitClauses();
            if (propagator.propagate() != nullptr) { clause_db.emptyClause(); }
            if (clause_db.hasEmptyClause()) break;

            if (!subsumption.subsume()) clause_db.emptyClause();

            if (subsumption.nTouched() > 0) occurence_list.cleanup();

            // propagateAndMaterializeUnitClauses();
            if (propagator.propagate() != nullptr) { clause_db.emptyClause(); }
            if (clause_db.hasEmptyClause()) break;

            if (!reduction.reduce()) clause_db.emptyClause();

            if (reduction.nTouched() > 0) occurence_list.cleanup();

            // propagateAndMaterializeUnitClauses();
            if (propagator.propagate() != nullptr) { clause_db.emptyClause(); }
            if (clause_db.hasEmptyClause()) break;

            if (!elimination.eliminate()) clause_db.emptyClause();

            if (elimination.nTouched() > 0) occurence_list.cleanup();

            if (verbosity > 1) {
                std::cout << "c " << std::this_thread::get_id() << ": Removed " << subsumption.nDuplicates << " Duplicate Clauses" << std::endl;
                std::cout << "c " << std::this_thread::get_id() << ": Subsumption subsumed " << subsumption.nSubsumed << " and strengthened " << subsumption.nStrengthened << " clauses" << std::endl;
                std::cout << "c " << std::this_thread::get_id() << ": Reduction strengthened " << reduction.nTouched() << " clauses" << std::endl;
                std::cout << "c " << std::this_thread::get_id() << ": Eliminiated " << elimination.nTouched() << " variables" << std::endl;
            }

            num = subsumption.nTouched() + elimination.nTouched() + reduction.nTouched();
            max = std::max(num, max);
        } 
    }

    void ipasir_callback(Clause* clause) {
        if (learntCallback != nullptr && clause->size() <= learntCallbackMaxLength) {
            std::vector<int> to_send;
            to_send.reserve(clause->size() + 1);
            for (Lit lit : *clause) {
                to_send.push_back((lit.var()+1)*(lit.sign()?-1:1));
            }
            to_send.push_back(0);
            learntCallback(learntCallbackState, to_send.data());
        }
    }

};

template<class TPropagate, class TLearning, class TBranching>
Solver<TPropagate, TLearning, TBranching>::Solver() : 
    // Basic Systems
    clause_db(),
    trail(),
    propagator(clause_db, trail),
	conflict_analysis(clause_db, trail),
    branch(clause_db, trail),
    restart(clause_db, trail),
    reduce(clause_db, trail),
    // verbosity
    verbosity(SolverOptions::verb), 
    // result
    result(),
    // pre- and inprocessing
    preprocessing_enabled(SolverOptions::opt_preprocessing),
    lastRestartWithInprocessing(0), inprocessingFrequency(SolverOptions::opt_inprocessing), 
    // interruption callback
    termCallbackState(nullptr), termCallback([](void*) -> int { return 0; }),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr)
{ }

template<class TPropagate, class TLearning, class TBranching>
Solver<TPropagate, TLearning, TBranching>::~Solver() {
}

template<class TPropagate, class TLearning, class TBranching>
void Solver<TPropagate, TLearning, TBranching>::clear() {
    clause_db.clear();
    propagator.clear();
    branch.clear();
}

template<class TPropagate, class TLearning, class TBranching>
void Solver<TPropagate, TLearning, TBranching>::init(const CNFProblem& problem, ClauseAllocator* allocator, bool lemma) {
    assert(trail.decisionLevel() == 0);

    // always initialize clause_db _first_
    clause_db.init(problem, allocator, lemma);

    for (Cl* clause : problem) {
        if (!clause_db.eliminated.has_eliminated_variables()) break;
        for (Lit lit : *clause) {
            if (clause_db.eliminated.is_eliminated(lit.var())) {
                std::vector<Cl> cor = clause_db.eliminated.undo(lit.var());
                for (Cl& cl : cor) clause_db.createClause(cl.begin(), cl.end());
            }
        }
    }

    trail.init(clause_db.nVars());
    propagator.init();
    conflict_analysis.init(clause_db.nVars());
    branch.init(problem);
}


template<class TPropagate, class TLearning, class TBranching>
lbool Solver<TPropagate, TLearning, TBranching>::search() {
    assert(!clause_db.hasEmptyClause());

    for (;;) {
        Clause* confl = (Clause*)propagator.propagate();

        if (confl != nullptr) { // CONFLICT
            if (trail.decisionLevel() == 0) {
                if (verbosity > 1) std::cout << "c Conflict found by propagation at level 0" << std::endl;
                return l_False;
            }
            
            conflict_analysis.handle_conflict(confl);
            restart.process_conflict();
            reduce.process_conflict();

            Clause* clause = clause_db.createClause(clause_db.result.learnt_clause.begin(), clause_db.result.learnt_clause.end(), clause_db.result.lbd);
            if (clause->size() > 2) {
                propagator.attachClause(clause);
            }

            if (clause_db.result.pickback_clause.size() > 0) {
                Clause* clause = clause_db.createClause(clause_db.result.pickback_clause.begin(), clause_db.result.pickback_clause.end(), clause_db.result.lbd);
                if (clause->size() > 2) {
                    propagator.attachClause(clause);
                }
            }

            trail.backtrack(clause_db.result.backtrack_level);
            branch.process_conflict();

            trail.propagate(clause->first(), clause);
            ipasir_callback(clause);
        }
        else {
            if (restart.trigger_restart() || reduce.trigger_reduce()) {
                return l_Undef;
            }
            
            Lit next = lit_Undef;
            while (trail.hasAssumptionsNotSet()) {
                Lit p = trail.nextAssumption();
                if (trail.value(p) == l_True) {
                    // std::cout << "c Assumption already satisfied " << p << std::endl;
                    trail.newDecisionLevel(); // Dummy decision level
                } 
                else if (trail.value(p) == l_False) {
                    std::cout << "c Conflict found during propagation of assumption " << p << std::endl;
                    result.setConflict(conflict_analysis.analyzeFinal(~p));
                    return l_False;
                } 
                else {
                    // std::cout << "c Propagating Assumption " << p << std::endl;
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

template<class TPropagate, class TLearning, class TBranching>
lbool Solver<TPropagate, TLearning, TBranching>::solve() {
    result.clear();
    trail.reset();
    propagator.reset();

    // this is kind-of needed now (came in with parallel version)
    propagateAndMaterializeUnitClauses();

    // prepare variable elimination for new set of assumptions
    for (Lit lit : trail.assumptions) if (clause_db.eliminated.is_eliminated(lit.var())) {
        std::vector<Cl> correction_set = clause_db.eliminated.undo(lit.var());
        for (Cl cl : correction_set) {
            Clause* clause = clause_db.createClause(cl.begin(), cl.end());
            if (clause->size() > 2) propagator.attachClause(clause);
            for (Lit lit : *clause) trail.setDecisionVar(lit.var(), true);
        }
    }
    
    if (this->preprocessing_enabled) {
        std::cout << "c Preprocessing ... " << std::endl;
        processClauseDatabase();
        propagator.reset();
    }
    
    std::cout << "c Searching ... " << std::endl;

    lbool status = clause_db.hasEmptyClause() ? l_False : l_Undef;

    while (status == l_Undef && termCallback(termCallbackState) == 0) {
        trail.backtrack(0);
        branch.add_back(trail.conflict_rbegin(), trail.rbegin());

        if (reduce.trigger_reduce()) {
            if (inprocessingFrequency > 0 && lastRestartWithInprocessing + inprocessingFrequency <= reduce.nReduceCalls()) { 
                std::cout << "c Inprocessing ... " << std::endl;
                lastRestartWithInprocessing = reduce.nReduceCalls();
                processClauseDatabase();
            }
            else {
                std::cout << "c Reducing ... " << std::endl;
                reduce.reduce();
            }            
            clause_db.reorganize();
            propagator.reset();
        }

        // multi-threaded unit-clauses fast-track
        // propagateAndMaterializeUnitClauses();
        if (propagator.propagate() != nullptr) { clause_db.emptyClause(); }

        if (clause_db.hasEmptyClause()) {
            status = l_False;
        } 
        else {
            if (verbosity > 1) std::cout << "c nClauses " << clause_db.size() << std::endl;
            status = search();
        }
    }

    if (status == l_Undef) {
        if (verbosity > 1) std::cout << "c " << std::this_thread::get_id() << ": Interrupted" << std::endl;
    }
    else {
        if (verbosity > 1) std::cout << "c " << std::this_thread::get_id() << ": " << (status == l_True ? "SAT" : "UNSAT") << std::endl;
    }
    
    result.setStatus(status);

    if (status == l_False) {
        if (result.getConflict().empty()) {
            clause_db.emptyClause();
        }
    }
    else if (status == l_True) {
        for (auto it = clause_db.eliminated.variables.rbegin(); it != clause_db.eliminated.variables.rend(); it++) {
            //std::cout << "Setting Eliminated Variable " << *it << std::endl;
            for (Cl& clause : clause_db.eliminated.clauses[*it]) {
                if (!trail.satisfies(clause.begin(), clause.end())) {
                    for (Lit lit : clause) { 
                        if (trail.value(lit) == l_Undef) {
                            trail.set_value(lit);
                        }
                    }
                }
            }
        }
        result.setModel(trail);
    }
    
    trail.backtrack(0);

    return status;
}

template<class TPropagate, class TLearning, class TBranching>
void Solver<TPropagate, TLearning, TBranching>::printStats() {
    std::cout << "c ************************* " << std::endl << std::left;
    std::cout << "c " << std::setw(20) << "restarts:" << nRestarts() << std::endl;
    std::cout << "c " << std::setw(20) << "conflicts:" << nConflicts() << std::endl;
    std::cout << "c " << std::setw(20) << "decisions:" << nDecisions() << std::endl;
    std::cout << "c " << std::setw(20) << "propagations:" << nPropagations() << std::endl;
    std::cout << "c " << std::setw(20) << "peak memory (mb):" << getPeakRSS()/(1024*1024) << std::endl;
    std::cout << "c " << std::setw(20) << "cpu time (s):" << get_cpu_time() << std::endl;
    std::cout << "c ************************* " << std::endl << std::left;
}

}

#endif
