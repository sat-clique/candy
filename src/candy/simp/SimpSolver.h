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
    virtual ~SimpSolver();

    // Problem specification:
    virtual Var newVar(bool polarity = true, bool dvar = true, double activity = 0.0); // Add a new variable with parameters specifying variable mode.
    virtual void addClauses(const CNFProblem& dimacs);

    bool eliminate() {
        return eliminate(use_asymm, use_elim);
    }

    virtual bool eliminate(bool use_asymm, bool use_elim);  // Perform variable elimination based simplification.

    virtual lbool solve();

    inline void enablePreprocessing() {
        preprocessing_enabled = true;
    }

    inline void disablePreprocessing() {
        preprocessing_enabled = false;
    }

    inline lbool solve(std::initializer_list<Lit> assumps) {
        return Solver<PickBranchLitT>::solve(assumps);
    }

    inline lbool solve(const vector<Lit>& assumps) {
        return Solver<PickBranchLitT>::solve(assumps);
    }

    inline bool isEliminated(Var v) const {
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
    unordered_map<Clause*, char> subsumption_queue_contains;
    vector<char> frozen;
    vector<char> eliminated;
    uint32_t bwdsub_assigns;
    uint32_t n_touched;

    // temporary
    std::vector<Lit> resolvent;

    // set these variables to frozen on init
    std::vector<Var> freezes;

    std::vector<Clause*> strengthened_clauses;
    std::unordered_map<Clause*, size_t> strengthened_sizes;
    std::unordered_map<Clause*, uint64_t> abstraction;

    // Main internal methods:
    inline void updateElimHeap(Var v) {
        // if (!frozen[v] && !isEliminated(v) && value(v) == l_Undef)
        if (elim_heap.inHeap(v) || (!frozen[v] && !isEliminated(v)
                                    && this->value(v) == l_Undef)) {
            elim_heap.update(v);
        }
    }

    // If a variable is frozen it will not be eliminated
    inline void setFrozenIntern(Var v, bool b) {
        frozen[v] = (char) b;
        if (!b) {
            updateElimHeap(v);
        }
    }

    void setupEliminate(bool full);
    void cleanupEliminate();

    void subsumptionQueueProtectedPush(Clause* cr);
    Clause* subsumptionQueueProtectedPop();

    void elimAttach(Clause* cr); // Attach a clause to occurrence lists for eliminate
    void elimDetach(Clause* cr, bool strict); // Detach a clause from occurrence lists for eliminate
    void elimDetach(Clause* cr, Lit lit, bool strict); // Detach a clause from lit's occurrence lists for eliminate

    bool asymm(Var v, Clause* cr);
    bool asymmVar(Var v);
    void gatherTouchedClauses();
    bool merge(const Clause& _ps, const Clause& _qs, Var v, vector<Lit>& out_clause);
    bool merge(const Clause& _ps, const Clause& _qs, Var v, size_t& size);
    bool backwardSubsumptionCheck();
    bool eliminateVar(Var v);
    void extendModel();

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
    subsumption_queue(),
    subsumption_queue_contains(),
    frozen(),
    eliminated(),
    bwdsub_assigns(0),
    n_touched(0),
    resolvent(),
    freezes(),
    strengthened_clauses(),
    strengthened_sizes(),
    abstraction() {
}

template<class PickBranchLitT>
SimpSolver<PickBranchLitT>::~SimpSolver() {
}

template<class PickBranchLitT>
Var SimpSolver<PickBranchLitT>::newVar(bool sign, bool dvar, double act) {
    Var v = Solver<PickBranchLitT>::newVar(sign, dvar, act);
    frozen.push_back((char) false);
    eliminated.push_back((char) false);
    return v;
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::addClauses(const CNFProblem& dimacs) {
    Solver<PickBranchLitT>::addClauses(dimacs);
    if (frozen.size() < this->nVars()) {
        frozen.resize(this->nVars(), (char) false);
        eliminated.resize(this->nVars(), (char) false);
    }
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
void SimpSolver<PickBranchLitT>::subsumptionQueueProtectedPush(Clause* cr) {
    if (!subsumption_queue_contains[cr]) {
        subsumption_queue.push_back(cr);
        subsumption_queue_contains[cr] = true;
    }
}

template<class PickBranchLitT>
Clause* SimpSolver<PickBranchLitT>::subsumptionQueueProtectedPop() {
    Clause* cr = subsumption_queue.front();
    subsumption_queue.pop_front();
    subsumption_queue_contains[cr] = false;
    return cr;
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::elimAttach(Clause* cr) {
#ifndef NDEBUG
    for (Lit lit : *cr) {
        assert(!isEliminated(var(lit)));
    }
#endif
    subsumption_queue.push_back(cr);
    subsumption_queue_contains[cr] = true;
    uint64_t clause_abstraction = 0;
    for (Lit lit : *cr) {
        clause_abstraction |= 1ull << (var(lit) % 64);
        occurs[var(lit)].push_back(cr);
        if (n_occ.size() > 0) { // elim initialized
            n_occ[toInt(lit)]++;
            touched[var(lit)] = 1;
            n_touched++;
            if (elim_heap.inHeap(var(lit))) {
                elim_heap.increase(var(lit));
            }
        }
    }
    abstraction[cr] = clause_abstraction;
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::elimDetach(Clause* cr, bool strict) {
    for (Lit lit : *cr) {
        elimDetach(cr, lit, strict);
    }
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::elimDetach(Clause* cr, Lit lit, bool strict) {
    if (strict) {
        occurs[var(lit)].erase(std::remove(occurs[var(lit)].begin(), occurs[var(lit)].end(), cr), occurs[var(lit)].end());
    }
    else {
        occurs.smudge(var(lit));
    }
    if (n_occ.size() > 0) { // elim initialized
        n_occ[toInt(lit)]--;
        updateElimHeap(var(lit));
    }
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::strengthenClause(Clause* cr, Lit l) {
    assert(this->decisionLevel() == 0);
    
    subsumptionQueueProtectedPush(cr);

    if (strengthened_sizes.count(cr) == 0) { // used to cleanup pages in clause-pool
        strengthened_sizes[cr] = cr->size();
        strengthened_clauses.push_back(cr);
    }
    
    this->detachClause(cr, true);
    cr->strengthen(l);

    this->certificate->added(cr->begin(), cr->end());

    elimDetach(cr, l, true);
    
    if (cr->size() == 1) {
        Lit unit = cr->first();
        elimDetach(cr, unit, true);
        cr->setDeleted();

        if (this->value(unit) == l_Undef) {
            this->uncheckedEnqueue(unit);
            return this->propagate() == nullptr;
        }
        else if (this->value(unit) == l_False) {
            return false;
        }
        else {
            this->vardata[var(unit)].reason = nullptr;
            this->vardata[var(unit)].level = 0;
            return true;
        }
    }
    else {
        this->attachClause(cr);

        uint64_t clause_abstraction = 0;
        for (Lit lit : *cr) {
            clause_abstraction |= 1ull << (var(lit) % 64);
        }
        abstraction[cr] = clause_abstraction;

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
    
    for (unsigned int i = 0; i < touched.size(); i++) {
        if (touched[i]) {
            const vector<Clause*>& cs = occurs.lookup(i);
            for (Clause* c : cs) {
                if (!c->isDeleted()) {
                    subsumptionQueueProtectedPush(c);
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
bool SimpSolver<PickBranchLitT>::backwardSubsumptionCheck() {
    assert(this->decisionLevel() == 0);

    Clause bwdsub_tmpunit({ lit_Undef });
    
    while (subsumption_queue.size() > 0 || bwdsub_assigns < this->trail_size) {
        if (this->asynch_interrupt) {
            break;
        }

        // Check top-level assignments by creating a dummy clause and placing it in the queue:
        if (subsumption_queue.size() == 0 && bwdsub_assigns < this->trail_size) {
            Lit l = this->trail[bwdsub_assigns++];
            bwdsub_tmpunit[0] = l;
            abstraction[&bwdsub_tmpunit] = 1ull << (var(l) % 64);
            subsumption_queue.push_back(&bwdsub_tmpunit);
        }

        Clause* cr = subsumptionQueueProtectedPop();
        
        if (cr->isDeleted()) {
            continue;
        }
        
        assert(cr->size() > 1 || this->value(cr->first()) == l_True); // Unit-clauses should have been propagated before this point.
        
        // Find best variable to scan:
        Var best = var(*std::min_element(cr->begin(), cr->end(), [this] (Lit l1, Lit l2) {
            return occurs[var(l1)].size() < occurs[var(l2)].size();
        }));

        // Search all candidates:
        vector<Clause*>& cs = occurs.lookup(best);
        for (unsigned int i = 0; i < cs.size(); i++) {
            Clause* csi = cs[i];
            if (csi != cr && (subsumption_lim == 0 || csi->size() < subsumption_lim)) {
                if ((abstraction[cr] & ~abstraction[csi]) != 0) continue;

                Lit l = cr->subsumes(*csi);

                if (l == lit_Undef) {
                    Statistics::getInstance().solverSubsumedInc();
                    this->removeClause(csi);
                    elimDetach(csi, false);
                }
                else if (l != lit_Error) {
                    Statistics::getInstance().solverDeletedInc();
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
    // Temporarily freeze variable. Otherwise, it would immediately end up on the queue again:
    bool was_frozen = frozen[v];
    frozen[v] = true;
    
    vector<Clause*> cls(occurs.lookup(v));
    
    if (this->value(v) != l_Undef || cls.size() == 0)
        return true;
    
    for (Clause* c : cls)
        if (!asymm(v, c))
            return false;
    
    bool ret = backwardSubsumptionCheck();
    frozen[v] = was_frozen;
    return ret;
}

namespace SimpSolverImpl {

void mkElimClause(vector<uint32_t>& elimclauses, Lit x);
void mkElimClause(vector<uint32_t>& elimclauses, Var v, Clause& c);

}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::eliminateVar(Var v) {
    assert(!isEliminated(v));
    
    if (this->value(v) != l_Undef || frozen[v]) {
        return true;
    }

    // split the occurrences into positive and negative:
    vector<Clause*>& cls = occurs.lookup(v);
    vector<Clause*> pos, neg;
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
    this->setDecisionVar(v, false);
    
    if (pos.size() > neg.size()) {
        for (Clause* c : neg) SimpSolverImpl::mkElimClause(elimclauses, v, *c);
        SimpSolverImpl::mkElimClause(elimclauses, mkLit(v));
    } else {
        for (Clause* c : pos) SimpSolverImpl::mkElimClause(elimclauses, v, *c);
        SimpSolverImpl::mkElimClause(elimclauses, ~mkLit(v));
    }
    
    size_t size = this->clauses.size();

    // produce clauses in cross product
    for (Clause* pc : pos) for (Clause* nc : neg) {
        if (merge(*pc, *nc, v, resolvent)) {
            this->certificate->added(resolvent.begin(), resolvent.end());
            this->addClause(resolvent);
        }
    }
    
    if (!this->isInConflictingState()) {
        for (auto it = this->clauses.begin() + size; it != this->clauses.end(); it++) {
            elimAttach(*it);
        }
        for (Clause* c : cls) {
            this->removeClause(c);
            elimDetach(c, false);
        }
        occurs[v].clear();
        this->watches[mkLit(v)].clear();
        this->watches[~mkLit(v)].clear();
        this->watchesBin[mkLit(v)].clear();
        this->watchesBin[~mkLit(v)].clear();

        return backwardSubsumptionCheck();
    }
    else {
        for (Clause* c : cls) {
            this->removeClause(c);
        }
        return false;
    }
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
    if (full) {
        n_occ.resize(2 * this->nVars(), 0);
        touched.resize(this->nVars(), 0);
        for (Var v = 0; v < static_cast<int>(this->nVars()); v++) {
            elim_heap.insert(v);
        }
    }

    occurs.init(this->nVars());

    // include persistent learnt clauses
    for (Clause* c : this->persist) {
        elimAttach(c);
    }
    for (Clause* c : this->learnts) {
        if (c->getLBD() <= 2) {
            elimAttach(c);
        }
    }
    for (Clause* c : this->clauses) {
        elimAttach(c);
    }

    // freeze assumptions and other externally set frozen variables
    for (Lit lit : this->assumptions) {
        setFrozenIntern(var(lit), true);
    }
    for (Var var : freezes) {
        setFrozenIntern(var, true);
    }
}

template<class PickBranchLitT>
void SimpSolver<PickBranchLitT>::cleanupEliminate() {
    for (Lit lit : this->assumptions) {
        setFrozenIntern(var(lit), false);
    }
    for (Var var : freezes) {
        setFrozenIntern(var, false);
    }

    n_occ.clear();
    touched.clear();
    elim_heap.clear();
    n_touched = 0;
    occurs.clear();
    subsumption_queue.clear();
    abstraction.clear();

    // force full cleanup
    this->rebuildOrderHeap();

    // cleanup strengthened clauses in pool
    for (Clause* clause : strengthened_clauses) {
        size_t size = strengthened_sizes[clause];
        if (!clause->isDeleted()) {
            // create clause in correct pool
            Clause* clean = new (this->allocator.allocate(clause->size())) Clause(*clause);
            if (clean->isLearnt()) {
                if (clean->size() == 2) {
                    this->persist.push_back(clean);
                } else {
                    clean->setLBD(std::min(clean->getLBD(), clean->size()));
                    this->learnts.push_back(clean);
                }
            } else {
                this->clauses.push_back(clean);
            }
            this->attachClause(clean);
            clause->setDeleted();
            this->detachClause(clause);
            if (this->locked(clause)) {
                this->vardata[var(clause->first())].reason = clean;
            }
        }
        clause->setSize(size);//restore original size for freeMarkedClauses
    }
    strengthened_clauses.clear();
    strengthened_sizes.clear();

    this->watches.cleanAll();
    this->watchesBin.cleanAll();
    this->freeMarkedClauses(this->clauses);
    this->freeMarkedClauses(this->learnts);
    this->freeMarkedClauses(this->persist);
}

template<class PickBranchLitT>
bool SimpSolver<PickBranchLitT>::eliminate(bool use_asymm, bool use_elim) {
    // prepare data-structures
    setupEliminate(use_asymm || use_elim);

    // only perform subsumption checks (inprocessing)
    if (!use_asymm && !use_elim) {
        if (subsumption_queue.size() > 0 || bwdsub_assigns < this->trail_size) {
            this->ok = backwardSubsumptionCheck();
        }
    }
    // either asymm or elim are true (preprocessing)
    else {
        while (n_touched > 0 || bwdsub_assigns < this->trail_size || elim_heap.size() > 0) {
            gatherTouchedClauses();
            
            if (subsumption_queue.size() > 0 || bwdsub_assigns < this->trail_size) {
                this->ok = backwardSubsumptionCheck();
            }

            while (!elim_heap.empty() && !this->isInConflictingState() && !this->asynch_interrupt) {
                Var elim = elim_heap.removeMin();

                if (isEliminated(elim) || this->value(elim) != l_Undef) {
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
    
    return this->ok;
}

}

#endif
