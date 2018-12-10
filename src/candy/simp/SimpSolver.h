/***************************************************************************************[SimpSolver.h]
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

#ifndef Glucose_SimpSolver_h
#define Glucose_SimpSolver_h

#include "candy/core/Solver.h"
#include "candy/core/Clause.h"
#include "candy/utils/System.h"
#include "candy/simp/Subsumption.h"
#include "candy/simp/VariableElimination.h"
#include "candy/frontend/CLIOptions.h"


namespace Candy {

/**
 * \tparam TBranchingHeuristic   the PickBranchLit type used to choose a
 *   strategy for determining decision (ie. branching) literals.
 *   TBranchingHeuristic must satisfy the following conditions:
 *    - TBranchingHeuristic must be a class type.
 *    - TBranchingHeuristic::Parameters must be a class type.
 *    - TBranchingHeuristic must have a zero-argument constructor.
 *    - TBranchingHeuristic must have a constructor taking a single argument of type
 *        const Parameters& params.
 *    - TBranchingHeuristic must be move-assignable.
 *    - There must be a specialization of Solver::pickBranchLit<TBranchingHeuristic>.
 */
template<class TClauseDatabase = ClauseDatabase, class TAssignment = Trail, class TPropagate = Propagate, class TLearning = ConflictAnalysis, class TBranching = VSIDS>
class SimpSolver: public Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching> {
public:
    SimpSolver();
    SimpSolver(TClauseDatabase& db, TAssignment& as, TPropagate& pr, TLearning& le, TBranching& br);
    virtual ~SimpSolver();

    bool eliminate() {
        return eliminate(use_asymm, use_elim);
    }

    virtual bool eliminate(bool use_asymm, bool use_elim);  // Perform variable elimination based simplification.

    virtual lbool solve();

    inline lbool solve(std::initializer_list<Lit> assumps) {
        return Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::solve(assumps);
    }

    inline lbool solve(const std::vector<Lit>& assumps) {
        return Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::solve(assumps);
    }

    inline bool isEliminated(Var v) const {
        return elimination.isEliminated(v);
    }

    Subsumption<TPropagate> subsumption;
    VariableElimination elimination;

    bool use_asymm;         // Shrink clauses by asymmetric branching.
    bool use_rcheck;        // Check if a clause is already implied. Prett costly, and subsumes subsumptions :)
    bool use_elim;          // Perform variable elimination.

protected:
    // Helper structures:
    struct ElimLt {
        const vector<int>& n_occ;
        explicit ElimLt(const std::vector<int>& no) : n_occ(no) { }

        uint64_t cost(Var x) const {
            return (uint64_t) n_occ[toInt(mkLit(x))] * (uint64_t) n_occ[toInt(~mkLit(x))];
        }

        // TODO: investigate this order alternative more.
        bool operator()(Var x, Var y) const {
            uint64_t c_x = cost(x);
            uint64_t c_y = cost(y);
            return c_x < c_y;// || c_x == c_y && x < y;
        }
    };

    // Solver state:
    std::vector<int> n_occ;
    Glucose::Heap<ElimLt> elim_heap;
    Stamp<uint8_t> frozen;

    inline bool subsumptionCheck() {
        bool ret = subsumption.backwardSubsumptionCheck();

        for (Lit lit : subsumption.reduced_literals) {
            elimDetach(lit);
        }
        subsumption.reduced_literals.clear();

        return ret;
    }

    inline void elimAttach(const Clause* cr) {
    #ifndef NDEBUG
        for (Lit lit : *cr) assert(!isEliminated(var(lit)));
    #endif
        if (n_occ.size() > 0) { // elim initialized
            for (Lit lit : *cr) {
                n_occ[toInt(lit)]++;
                subsumption.touch(var(lit));
                if (elim_heap.inHeap(var(lit))) {
                    elim_heap.increase(var(lit));
                }
            }
        }
    }

    inline void elimDetach(const Clause* cr) {
        if (n_occ.size() > 0) for (Lit lit : *cr) {
            elimDetach(lit);
        }
    }

    inline void elimDetach(Lit lit) {
        if (n_occ.size() > 0) { // elim initialized
            n_occ[toInt(lit)]--;
            if (elim_heap.inHeap(var(lit)) || (!frozen[var(lit)] && !isEliminated(var(lit)) && !this->trail.isAssigned(var(lit)))) {
                elim_heap.update(var(lit));
            }
        }
    }


    void setupEliminate(bool full);
    void cleanupEliminate();

    bool asymmVar(Var v);

    void extendModel();

    bool implied(const vector<Lit>& c);
};

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::SimpSolver() 
    : Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>(),
    subsumption(this->clause_db, this->trail, this->propagator, this->certificate),
    elimination(this->clause_db),
    use_asymm(SimpSolverOptions::opt_use_asymm),
    use_rcheck(SimpSolverOptions::opt_use_rcheck),
    use_elim(SimpSolverOptions::opt_use_elim),
    elim_heap(ElimLt(n_occ)), 
    frozen() {
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::SimpSolver(TClauseDatabase& db, TAssignment& as, TPropagate& pr, TLearning& le, TBranching& br) 
    : Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>(db, as, pr, le, br),
    subsumption(this->clause_db, this->trail, this->propagator, this->certificate),
    elimination(this->clause_db),
    use_asymm(SimpSolverOptions::opt_use_asymm),
    use_rcheck(SimpSolverOptions::opt_use_rcheck),
    use_elim(SimpSolverOptions::opt_use_elim),
    elim_heap(ElimLt(n_occ)), 
    frozen() {
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::~SimpSolver() {
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
lbool SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::solve() {
    lbool result = l_True;
    
    if (this->isInConflictingState()) {
        return l_False;
    }

    if (this->preprocessing_enabled) {
        result = lbool(eliminate());
    }
    
    if (result == l_True) {
        result = Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::solve();
    }
    
    if (result == l_True) {
        extendModel();
    }
    
    return result;
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
bool SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::implied(const vector<Lit>& c) {
    assert(this->decisionLevel() == 0);
    
    this->trail_lim.push_back(this->trail_size);
    for (Lit lit : c) {
        if (this->value(lit) == l_True) {
            this->trail.cancelUntil(0);
            return false;
        } 
        else if (this->value(lit) != l_False) {
            assert(this->value(lit) == l_Undef);
            this->trail.uncheckedEnqueue(~lit);
        }
    }
    
    bool result = this->propagator.propagate() != nullptr;
    this->trail.cancelUntil(0);
    return result;
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
bool SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::asymmVar(Var v) {
    assert(this->trail.decisionLevel() == 0);

    // Temporarily freeze variable. Otherwise, it would immediately end up on the queue again:
    bool was_frozen = frozen[v];
    frozen.set(v);
    
    const std::vector<Clause*>& clauses = this->clause_db.getOccurenceList(v);
    
    if (this->trail.isAssigned(v) || clauses.size() == 0) {
        return true;
    }
    
    for (size_t i = 0, size = clauses.size(); i < size; i++) {
        const Clause* clause = clauses[i];
        if (!clause->isDeleted() && !this->trail.satisfies(*clause)) {
            this->trail.newDecisionLevel();
            Lit l = lit_Undef;
            for (Lit lit : *clause) {
                if (var(lit) != v && !this->trail.isAssigned(var(lit))) {
                    this->trail.uncheckedEnqueue(~lit);
                } else {
                    l = lit;
                }
            }
            bool asymm = this->propagator.propagate() != nullptr;
            this->trail.cancelUntil(0);
            assert(l != lit_Undef);
            if (asymm) { // strengthen:
                elimDetach(l);
                this->propagator.detachClause(clause);
                this->clause_db.removeClause((Clause*)clause);

                std::vector<Lit> lits = clause->except(l);
                this->certificate.added(lits.begin(), lits.end());
                this->certificate.removed(clause->begin(), clause->end());
                
                if (lits.size() == 1) {
                    elimDetach(lits.front());
                    if (!this->trail.newFact(lits.front()) || this->propagator.propagate() != nullptr) {
                        return false;
                    }
                }
                else {
                    Clause* new_clause = this->clause_db.createClause(lits);
                    this->propagator.attachClause(new_clause);
                    subsumption.attach(new_clause);
                }
            }
        }
    }

    if (!was_frozen) frozen.unset(v);
    
    return subsumptionCheck();
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
void SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::extendModel() {
    elimination.extendModel(this->model);
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
void SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::setupEliminate(bool full) {
    frozen.grow(this->nVars());

    // freeze assumptions and other externally set frozen variables
    for (Lit lit : this->assumptions) {
        frozen.set(var(lit));
    }
    for (Var var : this->freezes) {
        frozen.set(var);
    }

    // include persistent learnt clauses
    this->clause_db.initOccurrenceTracking(this->nVars());
    subsumption.init(this->nVars());
    for (const Clause* c : this->clause_db) {
        elimAttach(c);
    }

    if (full) {
        n_occ.resize(2 * this->nVars(), 0);
        for (Var v = 0; v < static_cast<int>(this->nVars()); v++) {
            elim_heap.insert(v);
        }
    }
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
void SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::cleanupEliminate() {
    frozen.clear();
    n_occ.clear();
    elim_heap.clear();

    this->clause_db.stopOccurrenceTracking();

    subsumption.clear();

    // force full cleanup
    this->branch.notify_restarted(); // former rebuildOrderHeap
    this->clause_db.cleanup();
}

template<class TClauseDatabase, class TAssignment, class TPropagate, class TLearning, class TBranching>
bool SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>::eliminate(bool use_asymm, bool use_elim) {
    // prepare data-structures
    setupEliminate(use_asymm || use_elim);

    // only perform subsumption checks (inprocessing)
    if (!use_asymm && !use_elim) {
        if (subsumption.subsumption_queue.size() > 0 || subsumption.bwdsub_assigns < this->trail.size()) {
            this->ok = subsumptionCheck();
        }
    }
    // either asymm or elim are true (preprocessing)
    else {
        while (subsumption.hasTouchedClauses() || subsumption.bwdsub_assigns < this->trail.size() || elim_heap.size() > 0) {
            subsumption.gatherTouchedClauses();
            
            if (subsumption.subsumption_queue.size() > 0 || subsumption.bwdsub_assigns < this->trail.size()) {
                this->ok = subsumptionCheck();
            }

            while (!elim_heap.empty() && !this->isInConflictingState() && !this->asynch_interrupt) {
                Var elim = elim_heap.removeMin();

                if (isEliminated(elim) || this->trail.isAssigned(elim)) {
                    continue;
                }

                if (use_asymm && !this->isInConflictingState()) {
                    this->ok = asymmVar(elim);
                }

                if (use_elim && !this->isInConflictingState() && !this->trail.isAssigned(elim) && !frozen[elim]) {
                    bool was_eliminated = elimination.eliminateVar(elim);

                    if (was_eliminated) {
                        for (Cl& resolvent : elimination.resolvents) {
                            this->certificate.added(resolvent.begin(), resolvent.end());
                            if (resolvent.size() == 0) {
                                this->ok = false;
                            }
                            else if (resolvent.size() == 1) {
                                this->ok = this->trail.newFact(resolvent.front());
                            }
                            else {
                                Clause* clause = this->clause_db.createClause(resolvent);
                                this->propagator.attachClause(clause);
                                elimAttach(clause);
                                subsumption.attach(clause);
                            }
                        }

                        for (const Clause* c : elimination.resolved) {
                            this->certificate.removed(c->begin(), c->end());
                            this->propagator.detachClause(c);
                            this->clause_db.removeClause((Clause*)c);
                            elimDetach(c);
                        }

                        this->clause_db.cleanup();
                        this->branch.setDecisionVar(elim, false);
                        
                        if (this->isInConflictingState()) {
                            return false;
                        }
                        
                        this->ok = subsumptionCheck();
                    }
                }
            }

            if (this->isInConflictingState() || this->asynch_interrupt) {
                break;
            }
        }
    }

    cleanupEliminate();

    if (!this->ok) {
        this->certificate.proof();
    }
    
    return this->ok;
}

}

#endif
