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
#include <deque>

using namespace std;

namespace Candy {

class SimpSolver: public Solver {
public:
    SimpSolver();
    ~SimpSolver();

    // Problem specification:
    virtual Var newVar(bool polarity = true, bool dvar = true); // Add a new variable with parameters specifying variable mode.
    virtual bool addClause_(vector<Lit>& ps);
    bool substitute(Var v, Lit x);  // Replace all occurences of v with x (may cause a contradiction).
    bool eliminate(bool turn_off_elim = false);  // Perform variable elimination based simplification.
    virtual lbool solve();

    inline void enableSimplification() {
        use_simplification = true;
    }

    inline void disableSimplification() {
        use_simplification = false;
    }

    inline void cleanupSimplification() {
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

    inline bool addClause(const vector<Lit>& ps) {
        add_tmp.clear();
        add_tmp.insert(add_tmp.end(), ps.begin(), ps.end());
        return addClause_(add_tmp);
    }

    inline bool addClause(std::initializer_list<Lit> lits) {
        add_tmp.clear();
        add_tmp.insert(add_tmp.end(), lits.begin(), lits.end());
        return addClause_(add_tmp);
    }

    // If a variable is frozen it will not be eliminated
    inline void setFrozen(Var v, bool b) {
        frozen[v] = (char) b;
        if (use_simplification && !b) {
            updateElimHeap(v);
        }
    }

    inline bool isEliminated(Var v) const {
        return eliminated[v];
    }

    inline lbool solve(std::initializer_list<Lit> assumps) {
        assumptions.clear();
        assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
        return solve();
    }

    inline lbool solve(const std::vector<Lit>& assumps) {
        assumptions.clear();
        assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
        return solve();
    }

    // Mode of operation:
    size_t grow;              // Allow a variable elimination step to grow by a number of clauses (default to zero).
    size_t clause_lim;        // Variables are not eliminated if it produces a resolvent with a length above this limit. -1 means no limit.
    size_t subsumption_lim;   // Do not check if subsumption against a clause larger than this. -1 means no limit.

    bool use_asymm;         // Shrink clauses by asymmetric branching.
    bool use_rcheck;        // Check if a clause is already implied. Prett costly, and subsumes subsumptions :)
    bool use_elim;          // Perform variable elimination.
    bool use_simplification;

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
        assert(use_simplification);
        // if (!frozen[v] && !isEliminated(v) && value(v) == l_Undef)
        if (elim_heap.inHeap(v) || (!frozen[v] && !isEliminated(v) && value(v) == l_Undef)) {
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

}

#endif
