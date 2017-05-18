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
#include <deque>

using namespace std;

namespace Candy {

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
template<class PickBranchLitT=DefaultPickBranchLit>
class SimpSolver: public Solver<PickBranchLitT> {
public:
    SimpSolver();
    ~SimpSolver();

    // Problem specification:
    virtual Var newVar(bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.
    virtual bool addClause_(vector<Lit>& ps);
    bool substitute(Var v, Lit x);  // Replace all occurences of v with x (may cause a contradiction).
    bool eliminate(bool turn_off_elim = false);  // Perform variable elimination based simplification.
    virtual lbool solve();

    inline void enablePreprocessing() {
        preprocessing_enabled = true;
    }

    inline void disablePreprocessing() {
        preprocessing_enabled = false;
    }

    inline void cleanupPreprocessing() {
        touched.clear();
        occurs.clear();
        n_occ.clear();
        elim_heap.clear();
        subsumption_queue.clear();

        preprocessing_enabled = false;
        this->remove_satisfied = true;

        // Force full cleanup (this is safe and desirable since it only happens once):
        this->rebuildOrderHeap();
    }

    inline bool addClause(const vector<Lit>& ps) {
        this->add_tmp.clear();
        this->add_tmp.insert(this->add_tmp.end(),
                                               ps.begin(), ps.end());
        return addClause_(this->add_tmp);
    }

    inline bool addClause(std::initializer_list<Lit> lits) {
        this->add_tmp.clear();
        this->add_tmp.insert(this->add_tmp.end(),
                                               lits.begin(), lits.end());
        return addClause_(this->add_tmp);
    }

    // If a variable is frozen it will not be eliminated
    inline void setFrozen(Var v, bool b) {
        frozen[v] = (char) b;
        if (preprocessing_enabled && !b) {
            updateElimHeap(v);
        }
    }

    inline bool isEliminated(Var v) const {
        return eliminated[v];
    }

    inline lbool solve(std::initializer_list<Lit> assumps) {
        this->assumptions.clear();
        this->assumptions.insert(this->assumptions.end(),
                                                   assumps.begin(), assumps.end());
        return solve();
    }

    inline lbool solve(const std::vector<Lit>& assumps) {
        this->assumptions.clear();
        this->assumptions.insert(this->assumptions.end(),
                                                   assumps.begin(), assumps.end());
        return solve();
    }

    // Mode of operation:
    uint16_t subsumption_lim;   // Do not check if subsumption against a clause larger than this. 0 means no limit.
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

    struct ClauseDeleted {
        explicit ClauseDeleted() { }
        inline bool operator()(const Clause* cr) const {
            return cr->isDeleted();
        }
    };

    // Solver state:
    vector<uint32_t> elimclauses;
    vector<char> touched;
    OccLists<Var, Clause*, ClauseDeleted> occurs;
    vector<int> n_occ;
    Glucose::Heap<ElimLt> elim_heap;
    deque<Clause*> subsumption_queue;
    vector<char> frozen;
    vector<char> eliminated;
    uint32_t bwdsub_assigns;
    uint32_t n_touched;
    vector<Clause*> strengthend;

    // Main internal methods:
    inline void updateElimHeap(Var v) {
        assert(preprocessing_enabled);
        // if (!frozen[v] && !isEliminated(v) && value(v) == l_Undef)
        if (elim_heap.inHeap(v) || (!frozen[v] && !isEliminated(v)
                                    && this->value(v) == l_Undef)) {
            elim_heap.update(v);
        }
    }

    bool asymm(Var v, Clause* cr);
    bool asymmVar(Var v);
    void gatherTouchedClauses();
    bool merge(const Clause& _ps, const Clause& _qs, Var v, vector<Lit>& out_clause);
    bool merge(const Clause& _ps, const Clause& _qs, Var v, size_t& size);
    bool backwardSubsumptionCheck(bool verbose = false);
    bool eliminateVar(Var v);
    void extendModel();

    void removeClause(Clause* cr);
    bool strengthenClause(Clause* cr, Lit l);
    bool implied(const vector<Lit>& c);
};

/**
  * \brief A readily forward-declarable SimpSolver<>
  */
class DefaultSimpSolver : public SimpSolver<> {

};

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
extern IntOption opt_subsumption_lim;
}

//=================================================================================================
// Constructor/Destructor:

template<class PickBranchLitT>
SimpSolver<PickBranchLitT>::SimpSolver() :
    Solver<PickBranchLitT>(),
    subsumption_lim(SimpSolverOptions::opt_subsumption_lim),
    clause_lim(SimpSolverOptions::opt_clause_lim),
    grow(SimpSolverOptions::opt_grow),
    use_asymm(SimpSolverOptions::opt_use_asymm),
    use_rcheck(SimpSolverOptions::opt_use_rcheck),
    use_elim(SimpSolverOptions::opt_use_elim),
    preprocessing_enabled(true),
    occurs(ClauseDeleted()),
    elim_heap(ElimLt(n_occ)),
    bwdsub_assigns(0),
    n_touched(0),
    strengthend() {
    this->remove_satisfied = false;
}

template<class PickBranchLitT>
SimpSolver<PickBranchLitT>::~SimpSolver() {
}

template<class PickBranchLitT>
Var SimpSolver<PickBranchLitT>::newVar(bool sign, bool dvar) {
    Var v = Solver<PickBranchLitT>::newVar(sign, dvar);
    frozen.push_back((char) false);
    eliminated.push_back((char) false);
    
    if (preprocessing_enabled) {
        n_occ.push_back(0);
        n_occ.push_back(0);
        occurs.init(v);
        touched.push_back(0);
        elim_heap.insert(v);
    }
    return v;
}

template<class PickBranchLitT>
lbool SimpSolver<PickBranchLitT>::solve() {
    lbool result = l_True;
    
    if (preprocessing_enabled) {
        result = lbool(eliminate());
    }
    
    if (result == l_True) {
        // subsumption check finished, use activity
        for (Clause* c : this->clauses) {
            c->activity() = 0;
        }
        result = Solver<PickBranchLitT>::solve();
    }
    
    if (result == l_True) {
        extendModel();
    }
    
    return result;
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::addClause_(vector<Lit>& ps) {
    for (unsigned int i = 0; i < ps.size(); i++) {
        assert(!isEliminated(var(ps[i])));
    }

    size_t nclauses = this->clauses.size();
    
    if (use_rcheck && implied(ps)) {
        return true;
    }
    
    if (!Solver<PickBranchLitT>::addClause_(ps)) {
        return false;
    }
    
    if (preprocessing_enabled && this->clauses.size() == nclauses + 1) {
        Clause* cr = this->clauses.back();
        // NOTE: the clause is added to the queue immediately and then
        // again during 'gatherTouchedClauses()'. If nothing happens
        // in between, it will only be checked once. Otherwise, it may
        // be checked twice unnecessarily. This is an unfortunate
        // consequence of how backward subsumption is used to mimic
        // forward subsumption.
        subsumption_queue.push_back(cr);
        for (Lit lit : *cr) {
            occurs[var(lit)].push_back(cr);
            n_occ[toInt(lit)]++;
            touched[var(lit)] = 1;
            n_touched++;
            if (elim_heap.inHeap(var(lit))) {
                elim_heap.increase(var(lit));
            }
        }
    }
    
    return true;
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::removeClause(Clause* cr) {
    if (preprocessing_enabled) {
        for (Lit lit : *cr) {
            n_occ[toInt(lit)]--;
            updateElimHeap(var(lit));
            occurs.smudge(var(lit));
        }
    }
    Solver<PickBranchLitT>::removeClause(cr);
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::strengthenClause(Clause* cr, Lit l) {
    assert(this->decisionLevel() == 0);
    assert(preprocessing_enabled);
    
    // FIX: this is too inefficient but would be nice to have (properly implemented)
    // if (!find(subsumption_queue, &c))
    subsumption_queue.push_back(cr);
    
    this->detachClause(cr, true);
    cr->strengthen(l);
    cr->setLBD(cr->getLBD()+1); //use lbd to store original size
    strengthend.push_back(cr); //used to cleanup pages in clause-pool

    // detach
    occurs[var(l)].erase(std::remove(occurs[var(l)].begin(), occurs[var(l)].end(), cr), occurs[var(l)].end());
    n_occ[toInt(l)]--;
    updateElimHeap(var(l));

    this->certificate->added(cr->begin(), cr->end());
    
    if (cr->size() == 1) {
        cr->setDeleted();

        occurs[var(cr->first())].erase(std::remove(occurs[var(cr->first())].begin(), occurs[var(cr->first())].end(), cr), occurs[var(cr->first())].end());
        n_occ[toInt(cr->first())]--;
        updateElimHeap(var(cr->first()));

        return this->enqueue(cr->first()) && this->propagate() == nullptr;
    }
    else {
        this->attachClause(cr);

        assert(cr->size() > 1);
        return true;
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
    
    for (Clause* c : subsumption_queue)
        c->setFrozen(true);
    
    for (unsigned int i = 0; i < touched.size(); i++)
        if (touched[i]) {
            const vector<Clause*>& cs = occurs.lookup(i);
            for (Clause* c : cs) {
                if (!c->isFrozen() && !c->isDeleted()) {
                    subsumption_queue.push_back(c);
                    c->setFrozen(true);
                }
            }
            touched[i] = 0;
        }
    
    for (Clause* c : subsumption_queue)
        c->setFrozen(false);
    
    n_touched = 0;
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::implied(const vector<Lit>& c) {
    assert(this->decisionLevel() == 0);
    
    this->trail_lim.push_back(this->trail_size);
    for (Lit lit : c) {
        if (this->value(lit) == l_True) {
            this->cancelUntil(0);
            return false;
        } else if (this->value(lit) != l_False) {
            assert(this->value(lit) == l_Undef);
            this->uncheckedEnqueue(~lit);
        }
    }
    
    bool result = this->propagate() != nullptr;
    this->cancelUntil(0);
    return result;
}

// Backward subsumption + backward subsumption resolution
template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::backwardSubsumptionCheck(bool verbose) {
    int cnt = 0;
    int subsumed = 0;
    int deleted_literals = 0;
    Clause bwdsub_tmpunit({ lit_Undef });
    assert(this->decisionLevel() == 0);
    
    while (subsumption_queue.size() > 0 || bwdsub_assigns < this->trail_size) {
        // Empty subsumption queue and return immediately on user-interrupt:
        if (this->asynch_interrupt) {
            subsumption_queue.clear();
            bwdsub_assigns = this->trail_size;
            break;
        }
        
        // Check top-level assignments by creating a dummy clause and placing it in the queue:
        if (subsumption_queue.size() == 0 && bwdsub_assigns < this->trail_size) {
            Lit l = this->trail[bwdsub_assigns++];
            bwdsub_tmpunit[0] = l;
            bwdsub_tmpunit.calcAbstraction();
            subsumption_queue.push_back(&bwdsub_tmpunit);
        }
        
        Clause* cr = subsumption_queue.front();
        subsumption_queue.pop_front();
        
        if (cr->isDeleted()) {
            continue;
        }
        
        if (verbose && this->verbosity >= 2 && cnt++ % 1000 == 0) {
            printf("subsumption left: %10d (%10d subsumed, %10d deleted literals)\r", (int) subsumption_queue.size(), subsumed, deleted_literals);
        }
        
        assert(cr->size() > 1 || this->value(cr->first()) == l_True); // Unit-clauses should have been propagated before this point.
        
        // Find best variable to scan:
        Var best = var(cr->first());
        for (Lit lit : *cr) {
            if (occurs[var(lit)].size() < occurs[best].size()) {
                best = var(lit);
            }
        }
        
        // Search all candidates:
        vector<Clause*>& cs = occurs.lookup(best);
        for (unsigned int i = 0; i < cs.size(); i++) {
            Clause* csi = cs[i];
            if (cr->isDeleted()) {
                break;
            }
            else if (csi != cr && (subsumption_lim == 0 || csi->size() < subsumption_lim)) {
                Lit l = cr->subsumes(*csi);
                
                if (l == lit_Undef) {
                    subsumed++;
                    removeClause(csi);
                }
                else if (l != lit_Error) {
                    deleted_literals++;
                    
                    // this might modifiy occurs ...
                    if (!strengthenClause(csi, ~l)) {
                        return false;
                    }
                    
                    // ... occurs modified, so check candidate at index i again:
                    if (var(l) == best) {
                        i--;
                    }
                }
            }
        }
    }
    
    return true;
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::asymm(Var v, Clause* cr) {
    assert(this->decisionLevel() == 0);
    
    if (cr->isDeleted() || this->satisfied(*cr)) {
        return true;
    }
    
    this->trail_lim.push_back(this->trail_size);
    Lit l = lit_Undef;
    for (Lit lit : *cr) {
        if (var(lit) != v && this->value(lit) != l_False) {
            this->uncheckedEnqueue(~lit);
        }
        else {
            l = lit;
        }
    }
    
    if (this->propagate() != nullptr) {
        this->cancelUntil(0);
        if (!strengthenClause(cr, l)) {
            return false;
        }
    } else {
        this->cancelUntil(0);
    }
    
    return true;
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::asymmVar(Var v) {
    assert(preprocessing_enabled);
    
    const vector<Clause*>& cls = occurs.lookup(v);
    
    if (this->value(v) != l_Undef || cls.size() == 0)
        return true;
    
    for (Clause* c : cls)
        if (!asymm(v, c))
            return false;
    
    return backwardSubsumptionCheck();
}

namespace SimpSolverImpl {

void mkElimClause(vector<uint32_t>& elimclauses, Lit x);
void mkElimClause(vector<uint32_t>& elimclauses, Var v, Clause& c);

}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::eliminateVar(Var v) {
    assert(!frozen[v]);
    assert(!isEliminated(v));
    assert(this->value(v) == l_Undef);
    
    // Split the occurrences into positive and negative:
    vector<Clause*>& cls = occurs.lookup(v);
    vector<Clause*> pos, neg;
    for (Clause* cl : cls) {
        if (cl->contains(mkLit(v))) {
            pos.push_back(cl);
        }
        else {
            neg.push_back(cl);
        }
    }
    
    // Check wether the increase in number of clauses stays within the allowed ('grow'). Moreover, no
    // clause must exceed the limit on the maximal clause size:
    size_t cnt = 0;
    for (Clause* pc : pos) {
        for (Clause* nc : neg) {
            size_t clause_size = 0;
            if (merge(*pc, *nc, v, clause_size) && (++cnt > cls.size() + grow || (clause_lim > 0 && clause_size > clause_lim))) {
                return true;
            }
        }
    }
    
    // Delete and store old clauses:
    eliminated[v] = true;
    this->setDecisionVar(v, false);
    
    if (pos.size() > neg.size()) {
        for (Clause* c : neg)
            SimpSolverImpl::mkElimClause(elimclauses, v, *c);
        SimpSolverImpl::mkElimClause(elimclauses, mkLit(v));
    } else {
        for (Clause* c : pos)
            SimpSolverImpl::mkElimClause(elimclauses, v, *c);
        SimpSolverImpl::mkElimClause(elimclauses, ~mkLit(v));
    }
    
    // produce clauses in cross product
    std::vector<Lit>& resolvent = this->add_tmp;
    for (Clause* pc : pos) {
        for (Clause* nc : neg) {
            if (merge(*pc, *nc, v, resolvent) && !addClause_(resolvent)) {
                return false;
            } else {
                this->certificate->added(resolvent.begin(), resolvent.end());
            }
        }
    }
    
    for (Clause* c : cls) {
        removeClause(c);
    }
    
    // free references to eliminated variable
    occurs[v].clear();
    this->watches[mkLit(v)].clear();
    this->watches[~mkLit(v)].clear();
    
    return backwardSubsumptionCheck();
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::substitute(Var v, Lit x) {
    assert(!frozen[v]);
    assert(!isEliminated(v));
    assert(this->value(v) == l_Undef);
    
    if (!this->ok) {
        return false;
    }
    
    eliminated[v] = true;
    this->setDecisionVar(v, false);
    
    const vector<Clause*>& cls = occurs.lookup(v);
    for (Clause* c : cls) {
        this->add_tmp.clear();
        for (Lit lit : *c) {
            this->add_tmp.push_back(var(lit) == v ? x ^ sign(lit) : lit);
        }
        if (!addClause_(this->add_tmp)) {
            return this->ok = false;
        } else {
            this->certificate->added(this->add_tmp.begin(), this->add_tmp.end());
        }
        removeClause(c);
    }
    
    return true;
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
bool SimpSolver<PickBranchLitT>::eliminate(bool turn_off_elim) {
    vector<Var> extra_frozen;
    
    if (!this->simplify()) {
        this->ok = false;
        return false;
    }
    else if (!preprocessing_enabled) {
        return true;
    }
    
    if (this->clauses.size() > 4800000) {
        printf("c Too many clauses... No preprocessing\n");
        goto cleanup;
    }
    
    // Assumptions must be temporarily frozen to run variable elimination:
//    for (Lit lit : this->assumptions) {
//        Var v = var(lit);
//        assert(!isEliminated(v));
//        if (!frozen[v]) { // Freeze and store.
//            setFrozen(v, true);
//            extra_frozen.push_back(v);
//        }
//    }

    if (this->isIncremental()) {
        for (Var v = Solver<PickBranchLitT>::nbVarsInitialFormula; v < Solver<PickBranchLitT>::nVars(); v++) {
            assert(!isEliminated(v));
            if (!frozen[v]) { // Freeze and store.
                setFrozen(v, true);
                extra_frozen.push_back(v);
            }
        }
    }
    
    // Main simplification loop:
    while (n_touched > 0 || bwdsub_assigns < this->trail_size || elim_heap.size() > 0) {
        gatherTouchedClauses();
        
        if ((subsumption_queue.size() > 0 || bwdsub_assigns < this->trail_size) && !backwardSubsumptionCheck(true)) {
            this->ok = false;
            goto cleanup;
        }
        
        // Empty elim_heap and return immediately on user-interrupt:
        if (this->asynch_interrupt) {
            assert(bwdsub_assigns == this->trail_size);
            assert(subsumption_queue.size() == 0);
            assert(n_touched == 0);
            elim_heap.clear();
            goto cleanup;
        }
        
        while (!elim_heap.empty() && !this->asynch_interrupt) {
            Var elim = elim_heap.removeMin();
            
            if (isEliminated(elim) || this->value(elim) != l_Undef)
                continue;
            
            if (use_asymm) {
                // Temporarily freeze variable. Otherwise, it would immediately end up on the queue again:
                bool was_frozen = frozen[elim];
                frozen[elim] = true;
                if (!asymmVar(elim)) {
                    this->ok = false;
                    goto cleanup;
                }
                frozen[elim] = was_frozen;
            }
            
            // At this point, the variable may have been set by asymmetric branching, so check it
            // again. Also, don't eliminate frozen variables:
            if (use_elim && this->value(elim) == l_Undef && !frozen[elim] && !eliminateVar(elim)) {
                this->ok = false;
                goto cleanup;
            }
        }
        
        assert(subsumption_queue.size() == 0);
    }
    
cleanup:
    // Unfreeze the assumptions that were frozen:
    for (Var v : extra_frozen) {
        setFrozen(v, false);
    }
    
    // If no more simplification is needed, free all simplification-related data structures:
    if (turn_off_elim) {
        cleanupPreprocessing();
    }
    
    // cleanup strengthened clauses in pool (original size-offset was stored in lbd value)
    sort(strengthend.begin(), strengthend.end());
    strengthend.erase(std::unique(strengthend.begin(), strengthend.end()), strengthend.end());
    for (Clause* clause : strengthend) {
        if (!clause->isDeleted()) {
            // create clause in correct pool
            Clause* clean = new (this->allocator.allocate(clause->size())) Clause(std::vector<Lit>(clause->begin(), clause->end()));
            this->clauses.push_back(clean);
            this->attachClause(clean);
            this->detachClause(clause);
            clause->setDeleted();
        }
        clause->blow(clause->getLBD());//restore original size for freeMarkedClauses
    }
    
    occurs.cleanAll();
    this->watches.cleanAll();
    this->watchesBin.cleanAll();
    this->freeMarkedClauses(this->clauses);
    
    if (this->verbosity > 0 && elimclauses.size() > 0) {
        printf("c |  Eliminated clauses:     %10.2f Mb                                                     |\n", double(elimclauses.size() * sizeof(uint32_t)) / (1024*1024));
    }
    
    return this->ok;
}

}

#endif
