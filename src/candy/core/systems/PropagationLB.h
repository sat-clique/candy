/*************************************************************************************************
Candy -- Copyright (c) 2020, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef SRC_CANDY_CORE_PROPAGATION_LB_H_
#define SRC_CANDY_CORE_PROPAGATION_LB_H_

#include "candy/core/SolverTypes.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/Clause.h"
#include "candy/core/Trail.h"
#include "candy/core/systems/PropagationInterface.h"
#include <array>

namespace Candy {

struct LowerBound {
    int32_t lb; // lower bound unassigned
    Lit blocker; // possibly satisfied literal
    Clause* clause;

    LowerBound(Clause* cl) : 
        lb(0),
        blocker(cl->first()), 
        clause(cl) { }
};

class PropagationLB : public PropagationInterface {
private:
    ClauseDatabase& clause_db;
    Trail& trail;

    std::vector<std::vector<LowerBound*>> bounds;

    Memory<LowerBound> memory;

public:
    PropagationLB(ClauseDatabase& _clause_db, Trail& _trail) : 
        clause_db(_clause_db), trail(_trail), bounds(), memory() 
    {
        bounds.resize(Lit(clause_db.nVars(), true));
        for (Clause* clause : clause_db) {
            if (clause->size() > 2) {
                attachClause(clause);
            } 
        }
    }

    ~PropagationLB() { }

    void reset() override {
        for (auto& b : bounds) b.clear();
        for (Clause* clause : clause_db) {
            if (clause->size() > 2) {
                attachClause(clause);
            } 
        }
    }

    void attachClause(Clause* clause) override {
        assert(clause->size() > 2);
        LowerBound* lb = new (memory.allocate()) LowerBound(clause);
        for (Lit lit : *clause) {
            bounds[~lit].push_back(lb);
            if (trail.value(lit) != l_False) {
                lb->lb++;
            }
        }
    }

    void detachClause(Clause* clause) override {
        assert(clause->size() > 2);
        for (Lit lit : *clause) {
            bounds[~lit].erase(std::remove_if(bounds[~lit].begin(), bounds[~lit].end(), 
                [clause](LowerBound* lb){ return lb->clause == clause; }), bounds[~lit].end());
        }
    }

    inline Reason propagate_binary_clauses(Lit p) {
        for (Lit other : clause_db.binary_watchers[p]) {
            lbool val = trail.value(other);
            if (val == l_Undef) {
                trail.propagate(other, Reason(~p, other));
            }
            if (val == l_False) {
                return Reason(~p, other);
            }
        }
        return Reason();
    }

    /**************************************************************************************************
     *
     *  propagate : [void]  ->  [Clause*]
     *
     *  Description:
     *    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
     *    otherwise nullptr.
     *
     *    Post-conditions:
     *      * the propagation queue is empty, even if there was a conflict.
     **************************************************************************************************/
    Reason propagate_watched_clauses(Lit p) {
        for (LowerBound* bound : bounds[p]) {
            bound->lb--;
            if (bound->lb <= 1) {
                if (trail.value(bound->blocker) == l_True) {
                    bound->lb = 1; continue;
                }

                Clause* clause = bound->clause;

                if (clause->isDeleted()) continue;

                bound->lb = 0;
                for (Lit lit : *clause) {
                    lbool val = trail.value(lit);
                    if (val != l_False) {
                        bound->lb++;
                        bound->blocker = lit;
                        if (val == l_True) {
                            goto propagate_skip;
                        }
                    } 
                }

                if (bound->lb == 0) { // conflict
                    return Reason(clause);
                }
                else if (bound->lb == 1) { // unit
                    trail.propagate(bound->blocker, clause);
                }
            }
            propagate_skip:;
        }

        return Reason();
    }

    Reason propagate() override {
        Reason conflict;

        while (trail.qhead < trail.trail_size) {
            Lit p = trail[trail.qhead++];
            
            // Propagate binary clauses
            conflict = propagate_binary_clauses(p);
            if (conflict.exists()) return conflict;

            // Propagate other clauses
            conflict = propagate_watched_clauses(p);
            if (conflict.exists()) return conflict;
        }

        return Reason();
    }
};

}

#endif
