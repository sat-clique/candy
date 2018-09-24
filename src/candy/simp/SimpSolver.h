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

using namespace std;

namespace Candy {

//******************************************************************************
// SimpSolver<PickBranchLitT> implementation
//******************************************************************************

namespace SimpSolverOptions {
using namespace Glucose;

extern const char* _cat;
    
extern BoolOption opt_use_asymm;
extern BoolOption opt_use_rcheck;
extern BoolOption opt_use_elim;
extern IntOption opt_grow;
extern IntOption opt_clause_lim;
}

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
 *    - There must be a specialization of Solver::pickBranchLit<PickBranchLitT>.
 */
template<class PickBranchLitT = VSIDS>
class SimpSolver: public Solver<PickBranchLitT> {
public:
    SimpSolver();
    SimpSolver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_);
    SimpSolver(Conjectures conjectures, bool m_backbonesEnabled, RefinementHeuristic* rsar_filter_, bool filterOnlyBackbones_, uint64_t number);
    virtual ~SimpSolver();

    bool eliminate() {
        return eliminate(use_asymm, use_elim);
    }

    virtual bool eliminate(bool use_asymm, bool use_elim);  // Perform variable elimination based simplification.

    virtual lbool solve();

    void enablePreprocessing() {
        preprocessing_enabled = true;
    }

    void disablePreprocessing() {
        preprocessing_enabled = false;
    }

    inline lbool solve(std::initializer_list<Lit> assumps) {
        return Solver<PickBranchLitT>::solve(assumps);
    }

    inline lbool solve(const vector<Lit>& assumps) {
        return Solver<PickBranchLitT>::solve(assumps);
    }

    inline bool isEliminated(Var v) const {
        if (eliminated.size() < v) return false;
        return eliminated[v];
    }

    inline void setFrozen(Var v, bool freeze) {
        if (freeze) {
            freezes.push_back(v);
        } else {
            freezes.erase(std::remove(freezes.begin(), freezes.end(), v), freezes.end());
        }
    }

    // Mode of operation:
    Subsumption subsumption;

    uint8_t clause_lim;        // Variables are not eliminated if it produces a resolvent with a length above this limit. 0 means no limit.
    uint8_t grow;              // Allow a variable elimination step to grow by a number of clauses (default to zero).

    bool use_asymm;         // Shrink clauses by asymmetric branching.
    bool use_rcheck;        // Check if a clause is already implied. Prett costly, and subsumes subsumptions :)
    bool use_elim;          // Perform variable elimination.
    bool preprocessing_enabled; // do eliminate (via subsumption, asymm, elim)

protected:
    // Helper structures:
    struct ElimLt {
        const vector<int>& n_occ;
        explicit ElimLt(const vector<int>& no) : n_occ(no) { }

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
    std::vector<uint32_t> elimclauses;
    std::vector<char> touched;
    std::vector<int> n_occ;
    Glucose::Heap<ElimLt> elim_heap;
    Stamp<uint8_t> frozen;
    std::vector<char> eliminated;
    uint32_t n_touched;

    // set these variables to frozen on init
    std::vector<Var> freezes;

    // Main internal methods:
    inline void updateElimHeap(Var v) {
        // if (!frozen[v] && !isEliminated(v) && value(v) == l_Undef)
        if (elim_heap.inHeap(v) || (!frozen[v] && !isEliminated(v)
                                    && this->trail.value(v) == l_Undef)) {
            elim_heap.update(v);
        }
    }

    inline bool subsumptionCheck() {
        bool ret = subsumption.backwardSubsumptionCheck();

        for (Lit lit : subsumption.reduced_literals) {
            elimDetach(lit);
        }
        subsumption.reduced_literals.clear();

        return ret;
    }

    void setupEliminate(bool full);
    void cleanupEliminate();

    inline void elimAttach(Clause* cr); // Attach a clause to occurrence lists for eliminate
    inline void elimDetach(Clause* cr); // Detach a clause from occurrence lists for eliminate
    inline void elimDetach(Lit lit);

    bool asymm(Var v, Clause* cr);
    bool asymmVar(Var v);
    void gatherTouchedClauses();
    bool merge(const Clause& _ps, const Clause& _qs, Var v, vector<Lit>& out_clause);
    bool merge(const Clause& _ps, const Clause& _qs, Var v, size_t& size);
    bool eliminateVar(Var v);
    void extendModel();

    bool implied(const vector<Lit>& c);
};

//=================================================================================================
// Constructor/Destructor:

template<class PickBranchLitT>
SimpSolver<PickBranchLitT>::SimpSolver() : Solver<PickBranchLitT>(),
    subsumption(this->clause_db, this->trail, this->propagator, this->certificate), 
    clause_lim(SimpSolverOptions::opt_clause_lim),
    grow(SimpSolverOptions::opt_grow),
    use_asymm(SimpSolverOptions::opt_use_asymm),
    use_rcheck(SimpSolverOptions::opt_use_rcheck),
    use_elim(SimpSolverOptions::opt_use_elim),
    preprocessing_enabled(true),
    elim_heap(ElimLt(n_occ)),
    frozen(),
    eliminated(),
    n_touched(0),
    freezes() {
}

template<class PickBranchLitT>
SimpSolver<PickBranchLitT>::~SimpSolver() {
}

template<class PickBranchLitT>
lbool SimpSolver<PickBranchLitT>::solve() {
    lbool result = l_True;
    
    if (this->isInConflictingState()) {
        return l_False;
    }

    if (preprocessing_enabled) {
        result = lbool(eliminate());
    }
    
    if (result == l_True) {
        result = Solver<PickBranchLitT>::solve();
    }
    
    if (result == l_True) {
        extendModel();
    }
    
    return result;
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::elimAttach(Clause* cr) {
#ifndef NDEBUG
    for (Lit lit : *cr) {
        assert(!isEliminated(var(lit)));
    }
#endif
    if (n_occ.size() > 0) { // elim initialized
        for (Lit lit : *cr) {
            n_occ[toInt(lit)]++;
            touched[var(lit)] = 1;
            n_touched++;
            if (elim_heap.inHeap(var(lit))) {
                elim_heap.increase(var(lit));
            }
        }
    }
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::elimDetach(Clause* cr) {
    if (n_occ.size() > 0) for (Lit lit : *cr) {
        elimDetach(lit);
    }
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::elimDetach(Lit lit) {
    if (n_occ.size() > 0) { // elim initialized
        n_occ[toInt(lit)]--;
        updateElimHeap(var(lit));
    }
}

// Returns FALSE if clause is always satisfied ('out_clause' should not be used).
template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::merge(const Clause& _ps, const Clause& _qs, Var v, vector<Lit>& out_clause) {
    assert(_ps.contains(v));
    assert(_qs.contains(v));
    out_clause.clear();
    
    bool ps_smallest = _ps.size() < _qs.size();
    const Clause& ps = ps_smallest ? _qs : _ps;
    const Clause& qs = ps_smallest ? _ps : _qs;
    
    for (Lit qlit : qs) {
        if (var(qlit) != v) {
            auto p = find_if(ps.begin(), ps.end(), [qlit] (Lit plit) { return var(plit) == var(qlit); });
            if (p == ps.end()) {
                out_clause.push_back(qlit);
            }
            else if (*p == ~qlit) {
                return false;
            }
        }
    }
    
    for (Lit plit : ps) {
        if (var(plit) != v) {
            out_clause.push_back(plit);
        }
    }
    
    return true;
}

// Returns FALSE if clause is always satisfied.
template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::merge(const Clause& _ps, const Clause& _qs, Var v, size_t& size) {
    assert(_ps.contains(v));
    assert(_qs.contains(v));
    
    bool ps_smallest = _ps.size() < _qs.size();
    const Clause& ps = ps_smallest ? _qs : _ps;
    const Clause& qs = ps_smallest ? _ps : _qs;
    
    size = ps.size() - 1;
    
    for (Lit qlit : qs) {
        if (var(qlit) != v) {
            auto p = find_if(ps.begin(), ps.end(), [qlit] (Lit plit) { return var(plit) == var(qlit); });
            if (p == ps.end()) {
                size++;
            }
            else if (*p == ~qlit) {
                return false;
            }
        }
    }
    
    return true;
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::gatherTouchedClauses() {
    if (n_touched == 0)
        return;
    
    for (unsigned int i = 0; i < touched.size(); i++) {
        if (touched[i]) {
            const vector<Clause*>& cs = subsumption.occurs.lookup(i);
            for (Clause* c : cs) {
                if (!c->isDeleted()) {
                    subsumption.subsumptionQueueProtectedPush(c);
                }
            }
            touched[i] = 0;
        }
    }

    n_touched = 0;
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::implied(const vector<Lit>& c) {
    assert(this->decisionLevel() == 0);
    
    this->trail_lim.push_back(this->trail_size);
    for (Lit lit : c) {
        if (this->value(lit) == l_True) {
            this->trail.cancelUntil(0);
            return false;
        } else if (this->value(lit) != l_False) {
            assert(this->value(lit) == l_Undef);
            this->trail.uncheckedEnqueue(~lit);
        }
    }
    
    bool result = this->propagator.propagate(this->trail) != nullptr;
    this->trail.cancelUntil(0);
    return result;
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::asymm(Var v, Clause* cr) {
    assert(this->trail.decisionLevel() == 0);
    
    if (cr->isDeleted() || this->trail.satisfied(*cr)) {
        return true;
    }
    
    this->trail.trail_lim.push_back(this->trail.size());
    Lit l = lit_Undef;
    for (Lit lit : *cr) {
        if (var(lit) != v && this->trail.value(lit) != l_False) {
            this->trail.uncheckedEnqueue(~lit);
        }
        else {
            l = lit;
        }
    }
    
    if (this->propagator.propagate() != nullptr) {
        this->trail.cancelUntil(0);
        bool strengthen_ok = subsumption.strengthenClause(cr, l);

        for (Lit lit : subsumption.reduced_literals) {
            elimDetach(lit);
        }
        subsumption.reduced_literals.clear();

        if (!strengthen_ok) {
            return false;
        }
    } else {
        this->trail.cancelUntil(0);
    }
    
    return true;
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::asymmVar(Var v) {
    // Temporarily freeze variable. Otherwise, it would immediately end up on the queue again:
    bool was_frozen = frozen[v];
    frozen.set(v);
    
    vector<Clause*> cls(subsumption.occurs.lookup(v));
    
    if (this->trail.value(v) != l_Undef || cls.size() == 0)
        return true;
    
    for (Clause* c : cls)
        if (!asymm(v, c))
            return false;
    
    bool ret = subsumptionCheck();

    if (!was_frozen) frozen.unset(v);

    return ret;
}

namespace SimpSolverImpl {

void mkElimClause(vector<uint32_t>& elimclauses, Lit x);
void mkElimClause(vector<uint32_t>& elimclauses, Var v, Clause& c);

}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::eliminateVar(Var v) {
    assert(!isEliminated(v));
    
    if (this->trail.value(v) != l_Undef || frozen[v]) {
        return true;
    }

    // split the occurrences into positive and negative:
    std::vector<Clause*>& cls = subsumption.occurs.lookup(v);
    std::vector<Clause*> pos, neg;
    for (Clause* cl : cls) {
        if (cl->contains(mkLit(v))) {
            pos.push_back(cl);
        } else {
            neg.push_back(cl);
        }
    }
    
    // increase in number of clauses must stay within the allowed ('grow') and no clause must exceed the limit on the maximal clause size:
    size_t cnt = 0;
    for (Clause* pc : pos) for (Clause* nc : neg) {
        size_t clause_size = 0;
        if (merge(*pc, *nc, v, clause_size) && (++cnt > cls.size() + grow || (clause_lim > 0 && clause_size > clause_lim))) {
            return true;
        }
    }
    
    // delete and store old clauses:
    eliminated[v] = true;
    this->branch.setDecisionVar(v, false);
    
    if (pos.size() > neg.size()) {
        for (Clause* c : neg) SimpSolverImpl::mkElimClause(elimclauses, v, *c);
        SimpSolverImpl::mkElimClause(elimclauses, mkLit(v));
    } else {
        for (Clause* c : pos) SimpSolverImpl::mkElimClause(elimclauses, v, *c);
        SimpSolverImpl::mkElimClause(elimclauses, ~mkLit(v));
    }
    
    size_t size = this->clause_db.clauses.size();

    // produce clauses in cross product
    static std::vector<Lit> resolvent;
    resolvent.clear();
    for (Clause* pc : pos) for (Clause* nc : neg) {
        if (merge(*pc, *nc, v, resolvent)) {
            this->certificate.added(resolvent.begin(), resolvent.end());
            this->addClause(resolvent);
        }
    }
    for (auto it = this->clause_db.clauses.begin() + size; it != this->clause_db.clauses.end(); it++) {
        elimAttach(*it);
        subsumption.attach(*it);
    }

    // cleanup clauses to be removed
    for (Clause* c : cls) {
        this->certificate.removed(c->begin(), c->end());
        this->propagator.detachClause(c, false);
        if (this->trail.locked(c)) {
            this->trail.vardata[var(c->first())].reason = nullptr;
        }
        this->clause_db.removeClause(c);
        elimDetach(c);
        for (Lit lit : *c) subsumption.detach(c, lit, false);
    }
    this->propagator.cleanupWatchers();
    subsumption.occurs[v].clear();
    
    if (this->isInConflictingState()) {
        return false;
    }
    
    return subsumptionCheck();
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::extendModel() {
    this->model.resize(this->nVars());
    
    Lit x;
    for (int i = elimclauses.size()-1, j; i > 0; i -= j) {
        for (j = elimclauses[i--]; j > 1; j--, i--)
            if (this->modelValue(toLit(elimclauses[i])) != l_False)
                goto next;
        
        x = toLit(elimclauses[i]);
        this->model[var(x)] = lbool(!sign(x));
    next: ;
    }
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::setupEliminate(bool full) {
    if (frozen.size() < this->nVars()) {
        frozen.incSize(this->nVars());
    }
    if (eliminated.size() < this->nVars()) {
        eliminated.resize(this->nVars(), (char) false);
    }

    if (full) {
        n_occ.resize(2 * this->nVars(), 0);
        touched.resize(this->nVars(), 0);
        for (Var v = 0; v < static_cast<int>(this->nVars()); v++) {
            elim_heap.insert(v);
        }
    }

    subsumption.occurs.init(this->nVars());

    // include persistent learnt clauses
    for (Clause* c : this->clause_db.clauses) {
        subsumption.attach(c);
        elimAttach(c);
    }

    // freeze assumptions and other externally set frozen variables
    for (Lit lit : this->assumptions) {
        frozen.set(var(lit));
    }
    for (Var var : freezes) {
        frozen.set(var);
    }
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::cleanupEliminate() {
    frozen.clear();
    n_occ.clear();
    touched.clear();
    elim_heap.clear();
    n_touched = 0;
    subsumption.occurs.clear();
    subsumption.subsumption_queue.clear();
    subsumption.abstraction.clear();

    // force full cleanup
    this->branch.notify_restarted(); // former rebuildOrderHeap
    this->propagator.cleanupWatchers();
    this->clause_db.cleanup();
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::eliminate(bool use_asymm, bool use_elim) {
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
        while (n_touched > 0 || subsumption.bwdsub_assigns < this->trail.size() || elim_heap.size() > 0) {
            gatherTouchedClauses();
            
            if (subsumption.subsumption_queue.size() > 0 || subsumption.bwdsub_assigns < this->trail.size()) {
                this->ok = subsumptionCheck();
            }

            while (!elim_heap.empty() && !this->isInConflictingState() && !this->asynch_interrupt) {
                Var elim = elim_heap.removeMin();

                if (isEliminated(elim) || this->trail.value(elim) != l_Undef) {
                    continue;
                }

                if (use_asymm && !this->isInConflictingState()) {
                    this->ok = asymmVar(elim);
                }

                if (use_elim && !this->isInConflictingState()) {
                    this->ok = eliminateVar(elim);
                }
            }

            if (this->isInConflictingState() || this->asynch_interrupt) {
                break;
            }
        }
    }

    if (this->verbosity >= 2) {
        Statistics::getInstance().printSimplificationStats();
    }

    cleanupEliminate();

    if (!this->ok) {
        this->certificate.proof();
    }
    
    return this->ok;
}

}

#endif
