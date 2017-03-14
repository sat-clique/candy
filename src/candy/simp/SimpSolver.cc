/***************************************************************************************[SimpSolver.cc]
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

#include "candy/simp/SimpSolver.h"
#include "candy/utils/System.h"
#include "candy/core/Clause.h"

using namespace Candy;
using namespace Glucose;
using namespace std;

//=================================================================================================
// Options:

static const char* _cat = "SIMP";

static BoolOption opt_use_asymm(_cat, "asymm", "Shrink clauses by asymmetric branching.", false);
static BoolOption opt_use_rcheck(_cat, "rcheck", "Check if a clause is already implied. (costly)", false);
static BoolOption opt_use_elim(_cat, "elim", "Perform variable elimination.", true);
static IntOption opt_grow(_cat, "grow", "Allow a variable elimination step to grow by a number of clauses.", 0);
static IntOption opt_clause_lim(_cat, "cl-lim", "Variables are not eliminated if it produces a resolvent with a length above this limit. -1 means no limit", 20,
                IntRange(-1, INT32_MAX));
static IntOption opt_subsumption_lim(_cat, "sub-lim", "Do not check if subsumption against a clause larger than this. -1 means no limit.", 1000,
                IntRange(-1, INT32_MAX));
static DoubleOption opt_simp_garbage_frac(_cat, "simp-gc-frac",
                "The fraction of wasted memory allowed before a garbage collection is triggered during simplification.", 0.5,
                DoubleRange(0, false, HUGE_VAL, false));

//=================================================================================================
// Constructor/Destructor:

SimpSolver::SimpSolver() :
                Solver(), grow(opt_grow), clause_lim(opt_clause_lim), subsumption_lim(opt_subsumption_lim), simp_garbage_frac(
                                opt_simp_garbage_frac), use_asymm(opt_use_asymm), use_rcheck(opt_use_rcheck), use_elim(opt_use_elim), merges(0), asymm_lits(0), eliminated_vars(
                                0), elimorder(1), use_simplification(true), occurs(ClauseDeleted()), elim_heap(ElimLt(n_occ)), bwdsub_assigns(0), n_touched(0), strengthend() {
    remove_satisfied = false;
}

SimpSolver::~SimpSolver() {
}

Var SimpSolver::newVar(bool sign, bool dvar) {
    Var v = Solver::newVar(sign, dvar);
    frozen.push_back((char) false);
    eliminated.push_back((char) false);

    if (use_simplification) {
        n_occ.push_back(0);
        n_occ.push_back(0);
        occurs.init(v);
        touched.push_back(0);
        elim_heap.insert(v);
    }
    return v;
}

lbool SimpSolver::solve_(bool do_simp, bool turn_off_simp) {
    vector<Var> extra_frozen;
    lbool result = l_True;
    do_simp &= use_simplification;

    if (do_simp) {
        // Assumptions must be temporarily frozen to run variable elimination:
        for (Lit lit : assumptions) {
            Var v = var(lit);
            assert(!isEliminated(v));
            if (!frozen[v]) { // Freeze and store.
                setFrozen(v, true);
                extra_frozen.push_back(v);
            }
        }

        result = lbool(eliminate(turn_off_simp));
    }

    if (result == l_True) {
        // subsumption check finished, use activity
        for (Clause* c : clauses) {
            c->activity() = 0;
        }
        result = Solver::solve_();
    }

    if (result == l_True)
        extendModel();

    if (do_simp) { // Unfreeze the assumptions that were frozen:
        for (Var v : extra_frozen) {
            setFrozen(v, false);
        }
    }

    return result;
}

bool SimpSolver::addClause_(vector<Lit>& ps) {
#ifndef NDEBUG
    for (unsigned int i = 0; i < ps.size(); i++)
        assert(!isEliminated(var(ps[i])));
#endif
    unsigned int nclauses = clauses.size();

    if (use_rcheck && implied(ps))
        return true;

    if (!Solver::addClause_(ps))
        return false;

    if (use_simplification && clauses.size() == nclauses + 1) {
        Clause* cr = clauses.back();
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
            if (elim_heap.inHeap(var(lit)))
                elim_heap.increase(var(lit));
        }
    }

    return true;
}

void SimpSolver::removeClause(Clause* cr) {
    if (use_simplification) {
        for (Lit lit : *cr) {
            n_occ[toInt(lit)]--;
            updateElimHeap(var(lit));
            occurs.smudge(var(lit));
        }
    }
    Solver::removeClause(cr);
}

bool SimpSolver::strengthenClause(Clause* cr, Lit l) {
    assert(decisionLevel() == 0);
    assert(use_simplification);

    // FIX: this is too inefficient but would be nice to have (properly implemented)
    // if (!find(subsumption_queue, &c))
    subsumption_queue.push_back(cr);

    certificate.learntExcept(cr, l);

    if (cr->size() == 2) {
        removeClause(cr);
        cr->strengthen(l);
        cr->setLBD(cr->getLBD()+1); //use lbd to store original size
        Lit other = cr->first() == l ? cr->second() : cr->first();
        return enqueue(other) && propagate() == nullptr;
    }
    else {
        certificate.removed(cr);

        detachClause(cr, true);
        cr->strengthen(l);
        attachClause(cr);
        cr->setLBD(cr->getLBD()+1); //use lbd to store original size
        strengthend.push_back(cr); //used to cleanup pages in clause-pool

        occurs[var(l)].erase(std::remove(occurs[var(l)].begin(), occurs[var(l)].end(), cr), occurs[var(l)].end());
        n_occ[toInt(l)]--;
        updateElimHeap(var(l));

        assert(cr->size() > 1);
        return true;
    }
}

// Returns FALSE if clause is always satisfied ('out_clause' should not be used).
bool SimpSolver::merge(const Clause& _ps, const Clause& _qs, Var v, vector<Lit>& out_clause) {
    assert(_ps.contains(v));
    assert(_qs.contains(v));
    merges++;
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
bool SimpSolver::merge(const Clause& _ps, const Clause& _qs, Var v, int& size) {
    assert(_ps.contains(v));
    assert(_qs.contains(v));
    merges++;

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

void SimpSolver::gatherTouchedClauses() {
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

bool SimpSolver::implied(const vector<Lit>& c) {
    assert(decisionLevel() == 0);

    trail_lim.push_back(trail_size);
    for (Lit lit : c) {
        if (value(lit) == l_True) {
            cancelUntil(0);
            return false;
        } else if (value(lit) != l_False) {
            assert(value(lit) == l_Undef);
            uncheckedEnqueue(~lit);
        }
    }

    bool result = propagate() != nullptr;
    cancelUntil(0);
    return result;
}

// Backward subsumption + backward subsumption resolution
bool SimpSolver::backwardSubsumptionCheck(bool verbose) {
    int cnt = 0;
    int subsumed = 0;
    int deleted_literals = 0;
    Clause bwdsub_tmpunit({ lit_Undef });
    assert(decisionLevel() == 0);

    while (subsumption_queue.size() > 0 || bwdsub_assigns < trail_size) {
        // Empty subsumption queue and return immediately on user-interrupt:
        if (asynch_interrupt) {
            subsumption_queue.clear();
            bwdsub_assigns = trail_size;
            break;
        }

        // Check top-level assignments by creating a dummy clause and placing it in the queue:
        if (subsumption_queue.size() == 0 && bwdsub_assigns < trail_size) {
            Lit l = trail[bwdsub_assigns++];
            bwdsub_tmpunit[0] = l;
            bwdsub_tmpunit.calcAbstraction();
            subsumption_queue.push_back(&bwdsub_tmpunit);
        }

        Clause* cr = subsumption_queue.front();
        subsumption_queue.pop_front();

        if (cr->isDeleted())
            continue;

        if (verbose && verbosity >= 2 && cnt++ % 1000 == 0)
            printf("subsumption left: %10d (%10d subsumed, %10d deleted literals)\r", (int) subsumption_queue.size(), subsumed, deleted_literals);

        assert(cr->size() > 1 || value((*cr)[0]) == l_True); // Unit-clauses should have been propagated before this point.

        // Find best variable to scan:
        Var best = var((*cr)[0]);
        for (unsigned int i = 1; i < cr->size(); i++)
            if (occurs[var((*cr)[i])].size() < occurs[best].size())
                best = var((*cr)[i]);

        // Search all candidates:
        vector<Clause*>& cs = occurs.lookup(best);
        for (unsigned int i = 0; i < cs.size(); i++) {
            Clause* csi = cs[i];
            if (cr->isDeleted()) {
                break;
            }
            else if (csi != cr && (subsumption_lim == -1 || (int)csi->size() < subsumption_lim)) {
                Lit l = cr->subsumes(*csi);

                if (l == lit_Undef) {
                    subsumed++;
                    removeClause(csi);
                }
                else if (l != lit_Error) {
                    deleted_literals++;

                    if (!strengthenClause(csi, ~l))
                        return false;

                    // Did current candidate get deleted from cs? Then check candidate at index j again:
                    if (var(l) == best)
                        i--;
                }
            }
        }
    }

    return true;
}

bool SimpSolver::asymm(Var v, Clause* cr) {
    assert(decisionLevel() == 0);

    if (cr->isDeleted() || satisfied(*cr)) {
        return true;
    }

    trail_lim.push_back(trail_size);
    Lit l = lit_Undef;
    for (Lit lit : *cr) {
        if (var(lit) != v && value(lit) != l_False) {
            uncheckedEnqueue(~lit);
        }
        else {
            l = lit;
        }
    }

    if (propagate() != nullptr) {
        cancelUntil(0);
        asymm_lits++;
        if (!strengthenClause(cr, l)) {
            return false;
        }
    } else {
        cancelUntil(0);
    }

    return true;
}

bool SimpSolver::asymmVar(Var v) {
    assert(use_simplification);

    const vector<Clause*>& cls = occurs.lookup(v);

    if (value(v) != l_Undef || cls.size() == 0)
        return true;

    for (Clause* c : cls)
        if (!asymm(v, c))
            return false;

    return backwardSubsumptionCheck();
}

static void mkElimClause(vector<uint32_t>& elimclauses, Lit x) {
    elimclauses.push_back(toInt(x));
    elimclauses.push_back(1);
}

static void mkElimClause(vector<uint32_t>& elimclauses, Var v, Clause& c) {
    assert(c.contains(v));
    uint32_t first = elimclauses.size();

    // Copy clause to elimclauses-vector
    for (Lit lit : c) {
        if (var(lit) != v || first == elimclauses.size()) {
            elimclauses.push_back(lit);
        } else {
            // Swap such that 'v' will occur first in the clause:
            elimclauses.push_back(elimclauses[first]);
            elimclauses[first] = lit;
        }
    }

    // Store the length of the clause last:
    elimclauses.push_back(c.size());
}

bool SimpSolver::eliminateVar(Var v) {
    assert(!frozen[v]);
    assert(!isEliminated(v));
    assert(value(v) == l_Undef);

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
    // clause must exceed the limit on the maximal clause size (if it is set):
    int cnt = 0;
    for (Clause* pc : pos) {
        for (Clause* nc : neg) {
            int clause_size = 0;
            if (merge(*pc, *nc, v, clause_size) && (++cnt > (int)cls.size() + grow || (clause_lim != -1 && clause_size > clause_lim))) {
                return true;
            }
        }
    }

    // Delete and store old clauses:
    eliminated[v] = true;
    setDecisionVar(v, false);
    eliminated_vars++;

    if (pos.size() > neg.size()) {
        for (Clause* c : neg)
            mkElimClause(elimclauses, v, *c);
        mkElimClause(elimclauses, mkLit(v));
    } else {
        for (Clause* c : pos)
            mkElimClause(elimclauses, v, *c);
        mkElimClause(elimclauses, ~mkLit(v));
    }

    // Produce clauses in cross product:
    vector<Lit>& resolvent = add_tmp;
    for (Clause* pc : pos)
        for (Clause* nc : neg)
            if (merge(*pc, *nc, v, resolvent) && !addClause_(resolvent))
                return false;

    for (Clause* c : cls)
        removeClause(c);

    // Free occurs list for this variable:
    occurs[v].clear();
    // Free watchers lists for this variable:
    watches[mkLit(v)].clear();
    watches[~mkLit(v)].clear();

    return backwardSubsumptionCheck();
}

bool SimpSolver::substitute(Var v, Lit x) {
    assert(!frozen[v]);
    assert(!isEliminated(v));
    assert(value(v) == l_Undef);

    if (!ok) {
        return false;
    }

    eliminated[v] = true;
    setDecisionVar(v, false);

    const vector<Clause*>& cls = occurs.lookup(v);
    for (Clause* c : cls) {
        add_tmp.clear();
        for (Lit lit : *c) {
            add_tmp.push_back(var(lit) == v ? x ^ sign(lit) : lit);
        }
        if (!addClause_(add_tmp)) {
            return ok = false;
        }
        removeClause(c);
    }

    return true;
}

void SimpSolver::extendModel() {
    model.resize(nVars());

    Lit x;
    for (int i = elimclauses.size()-1, j; i > 0; i -= j) {
        for (j = elimclauses[i--]; j > 1; j--, i--)
            if (modelValue(toLit(elimclauses[i])) != l_False)
                goto next;

        x = toLit(elimclauses[i]);
        model[var(x)] = lbool(!sign(x));
        next: ;
    }
}

bool SimpSolver::eliminate(bool turn_off_elim) {
    if (!simplify()) {
        ok = false;
        return false;
    }
    else if (!use_simplification) {
        return true;
    }

    if (clauses.size() > 4800000) {
        printf("c Too many clauses... No preprocessing\n");
        goto cleanup;
    }

    // Main simplification loop:
    while (n_touched > 0 || bwdsub_assigns < trail_size || elim_heap.size() > 0) {
        gatherTouchedClauses();

        if ((subsumption_queue.size() > 0 || bwdsub_assigns < trail_size) && !backwardSubsumptionCheck(true)) {
            ok = false;
            goto cleanup;
        }

        // Empty elim_heap and return immediately on user-interrupt:
        if (asynch_interrupt) {
            assert(bwdsub_assigns == trail_size);
            assert(subsumption_queue.size() == 0);
            assert(n_touched == 0);
            elim_heap.clear();
            goto cleanup;
        }

        while (!elim_heap.empty() && !asynch_interrupt) {
            Var elim = elim_heap.removeMin();

            if (isEliminated(elim) || value(elim) != l_Undef)
                continue;

            if (use_asymm) {
                // Temporarily freeze variable. Otherwise, it would immediately end up on the queue again:
                bool was_frozen = frozen[elim];
                frozen[elim] = true;
                if (!asymmVar(elim)) {
                    ok = false;
                    goto cleanup;
                }
                frozen[elim] = was_frozen;
            }

            // At this point, the variable may have been set by asymmetric branching, so check it
            // again. Also, don't eliminate frozen variables:
            if (use_elim && value(elim) == l_Undef && !frozen[elim] && !eliminateVar(elim)) {
                ok = false;
                goto cleanup;
            }
        }

        assert(subsumption_queue.size() == 0);
    }
    cleanup:

    // If no more simplification is needed, free all simplification-related data structures:
    if (turn_off_elim) {
        touched.clear();
        occurs.clear();
        n_occ.clear();
        elim_heap.clear();
        subsumption_queue.clear();

        use_simplification = false;
        remove_satisfied = true;

        // Force full cleanup (this is safe and desirable since it only happens once):
        rebuildOrderHeap();
    }

    // cleanup strengthened clauses in pool (original size-offset was stored in lbd value)
    sort(strengthend.begin(), strengthend.end());
    strengthend.erase(std::unique(strengthend.begin(), strengthend.end()), strengthend.end());
    for (Clause* clause : strengthend) {
        if (!clause->isDeleted()) {
            // create clause in correct pool
            Clause* clean = new (clause->size()) Clause(std::vector<Lit>(clause->begin(), clause->end()), clause->isLearnt());
            clauses.push_back(clean);
            attachClause(clean);
            detachClause(clause, true);
            clause->setDeleted();
        }
        clause->blow(clause->getLBD());//restore original size for freeMarkedClauses
        clause->setLBD(0);//be paranoid
    }

    occurs.cleanAll();
    watches.cleanAll();
    watchesBin.cleanAll();
    freeMarkedClauses(clauses);
    
    if (verbosity >= 0 && elimclauses.size() > 0)
        printf("c |  Eliminated clauses:     %10.2f Mb                                                                |\n", 
               double(elimclauses.size() * sizeof(uint32_t)) / (1024*1024));

    return ok;
}
