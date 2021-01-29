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

#include "candy/core/systems/LearningInterface.h"
#include "candy/core/systems/Propagation2WL.h"
#include "candy/core/systems/BranchingVSIDS.h"
#include "candy/core/Restart.h"
#include "candy/core/ReduceDB.h"

#include "candy/simplification/Subsumption.h"
#include "candy/simplification/OccurenceList.h"
#include "candy/simplification/VariableElimination.h"

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

template<class TPropagation = Propagation2WL, class TLearning = Learning1UIP, class TBranching = BranchingVSIDS> 
class Solver : public CandySolverInterface {
protected:
    ClauseDatabase clause_db;
    Trail trail;

    TPropagation propagation;
    TLearning learning;
    TBranching branching;

    Restart restart;
    ReduceDB reduce;

    Subsumption subsumption;
    VariableElimination elimination;

    unsigned int verbosity;

    CandySolverResult result;

    bool preprocessing_enabled;

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

public:
    Solver() : clause_db(), trail(),
        // subsystems
        propagation(clause_db, trail),
        learning(clause_db, trail),
        branching(clause_db, trail),
        restart(clause_db, trail),
        reduce(clause_db, trail),
        subsumption(clause_db, trail),
        elimination(clause_db, trail),
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

    ~Solver() { }

    void clear() override {
        clause_db.clear();
        propagation.clear();
        branching.clear();
    }

    void init(const CNFProblem& problem, ClauseAllocator* allocator = nullptr, bool lemma = true) override  {
        assert(trail.decisionLevel() == 0);

        // always initialize clause_db _first_
        clause_db.init(problem, allocator, lemma);

        for (Cl* clause : problem) {
            if (!elimination.has_eliminated_variables()) break;
            for (Lit lit : *clause) {
                if (elimination.is_eliminated(lit.var())) {
                    std::vector<Cl> cor = elimination.undo(lit.var());
                    for (Cl& cl : cor) clause_db.createClause(cl.begin(), cl.end());
                }
            }
        }

        trail.init(clause_db.nVars());
        propagation.init();
        learning.init(clause_db.nVars());
        branching.init(problem);
        subsumption.init();
        elimination.init();
    }

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

    BranchingInterface* getBranchingSystem() override {
        return &branching;
    }

    LearningInterface* getLearningSystem() override {
        return &learning;
    }

    PropagationInterface* getPropagationSystem() override {
        return &propagation;
    }

    unsigned int nVars() const override {
        return clause_db.nVars();
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

    void printStats() override;

    void setTermCallback(void* state, int (*termCallback)(void* state)) override {
        this->termCallbackState = state;
        this->termCallback = termCallback;
    }

    void setLearntCallback(void* state, int max_length, void (*learntCallback)(void* state, int* clause)) override { 
        this->learntCallbackState = state;
        this->learntCallbackMaxLength = max_length;
        this->learntCallback = learntCallback;
    }

private:
    // pre- and inprocessing
    void processClauseDatabase() {
        assert(trail.decisionLevel() == 0);

        OccurenceList occurence_list { clause_db };        

        unsigned int num = 1;
        unsigned int max = 0;
        double simplification_threshold_factor = 0.1;
        while (num > max * simplification_threshold_factor && termCallback(termCallbackState) == 0) {
            if (num > 100) occurence_list.cleanup();

            if (clause_db.hasEmptyClause() || propagation.propagate().exists()) break;

            subsumption.subsume(occurence_list);

            if (clause_db.hasEmptyClause() || propagation.propagate().exists()) break;

            elimination.eliminate(occurence_list);

            num = subsumption.nTouched() + elimination.nTouched();
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

template<class TPropagation, class TLearning, class TBranching>
lbool Solver<TPropagation, TLearning, TBranching>::search() {
    assert(!clause_db.hasEmptyClause());

    for (;;) {
        Reason confl = propagation.propagate();

        if (confl.exists()) { // CONFLICT
            if (trail.decisionLevel() == 0) {
                if (verbosity > 1) std::cout << "c Conflict found by propagation at level 0" << std::endl;
                return l_False;
            }
            
            learning.handle_conflict(confl);
            restart.process_conflict();
            reduce.process_conflict();

            Clause* clause = clause_db.createClause(clause_db.result.learnt_clause.begin(), clause_db.result.learnt_clause.end(), clause_db.result.lbd);
            if (clause->size() > 2) {
                propagation.attachClause(clause);
            }

            trail.backtrack(clause_db.result.backtrack_level);
            branching.process_conflict();

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
                    trail.newDecisionLevel(); // Dummy decision level
                } 
                else if (trail.value(p) == l_False) {
                    std::cout << "c Conflict found during propagation of assumption " << p << std::endl;
                    result.setConflict(learning.analyzeFinal(~p));
                    return l_False;
                } 
                else {
                    next = p;
                    break;
                }
            }

            if (next == lit_Undef) {
                next = branching.pickBranchLit();
                if (next == lit_Undef) { // Model found
                    return l_True;
                }
            }
            
            trail.newDecisionLevel();
            trail.decide(next);
        }
    }
    return l_Undef; // not reached
}

template<class TPropagation, class TLearning, class TBranching>
lbool Solver<TPropagation, TLearning, TBranching>::solve() {
    result.clear();
    trail.reset();
    propagation.reset();

    // materialized unit-clauses for sharing (Todo: Refactor)
    for (Clause* clause : clause_db.getUnitClauses()) {
        if (!trail.fact(clause->first())) clause_db.emptyClause();
    }
    if (propagation.propagate().exists()) { clause_db.emptyClause(); }

    // prepare variable elimination for new set of assumptions
    for (Lit lit : trail.assumptions) if (elimination.is_eliminated(lit.var())) {
        std::vector<Cl> correction_set = elimination.undo(lit.var());
        for (Cl cl : correction_set) {
            Clause* clause = clause_db.createClause(cl.begin(), cl.end());
            if (clause->size() > 2) propagation.attachClause(clause);
            for (Lit lit : *clause) trail.setDecisionVar(lit.var(), true);
        }
    }
    
    if (this->preprocessing_enabled) {
        std::cout << "c Preprocessing ... " << std::endl;
        processClauseDatabase();
        propagation.reset();
        // materialized unit-clauses for sharing (Todo: Refactor)
        for (Clause* clause : clause_db.getUnitClauses()) {
            if (!trail.fact(clause->first())) clause_db.emptyClause();
        }
        if (propagation.propagate().exists()) { clause_db.emptyClause(); }
    }

    lbool status = clause_db.hasEmptyClause() ? l_False : l_Undef;

    while (status == l_Undef && termCallback(termCallbackState) == 0) {
        trail.backtrack(0);
        branching.add_back(trail.conflict_rbegin(), trail.rbegin());

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
            propagation.reset();
            // materialized unit-clauses for sharing (Todo: Refactor)
            for (Clause* clause : clause_db.getUnitClauses()) {
                if (!trail.fact(clause->first())) clause_db.emptyClause();
            }
        }

        if (propagation.propagate().exists()) { clause_db.emptyClause(); }

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
        for (auto it = elimination.variables.rbegin(); it != elimination.variables.rend(); it++) {
            //std::cout << "Setting Eliminated Variable " << *it << std::endl;
            for (Cl& clause : elimination.clauses[*it]) {
                if (!trail.satisfies(clause.begin(), clause.end())) {
                    for (Lit lit : clause) { 
                        if (lit.var() == *it) {
                            trail.set_value(lit);
                        }
                    }
                }
            }
            if (trail.value(*it) == l_Undef) {
                trail.set_value(Lit(*it));
            }
        }
        result.setModel(trail);
    }
    
    trail.backtrack(0);

    return status;
}


template<class TPropagation, class TLearning, class TBranching>
void Solver<TPropagation, TLearning, TBranching>::printStats() {
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
