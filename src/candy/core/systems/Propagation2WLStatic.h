/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef SRC_CANDY_CORE_PROPAGATION2WLSTATIC_H_
#define SRC_CANDY_CORE_PROPAGATION2WLSTATIC_H_

#include "candy/core/SolverTypes.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/Clause.h"
#include "candy/core/Trail.h"
#include "candy/mtl/Memory.h"
#include "candy/core/systems/PropagationInterface.h"
#include <array>

namespace Candy {

class Propagation2WLStatic : public PropagationInterface {
    struct Watcher {
        Clause* cref;
        Lit watch0;
        Lit watch1;

        Watcher(Clause* clause, Lit one, Lit two)
         : cref(clause), watch0(one), watch1(two) {}
    };

private:
    ClauseDatabase& clause_db;
    Trail& trail;

    std::vector<std::vector<Watcher*>> watchers;

    Memory<Watcher> memory;

public:
    Propagation2WLStatic(ClauseDatabase& _clause_db, Trail& _trail) : 
        clause_db(_clause_db), trail(_trail), watchers(), memory() 
    { 
        watchers.resize(Lit(clause_db.nVars(), true));
        for (Clause* clause : clause_db) {
            if (clause->size() > 2) {
                attachClause(clause);
            } 
        }
    }

    void reset() override {
        for (auto& w : watchers) w.clear();
        for (Clause* clause : clause_db) {
            if (clause->size() > 2) {
                attachClause(clause);
            } 
        }
    }

    void attachClause(Clause* clause) override {
        assert(clause->size() > 2);
        Watcher* watcher = new (memory.allocate()) Watcher(clause, clause->first(), clause->second());
        watchers[~(clause->first())].push_back(watcher);
        watchers[~(clause->second())].push_back(watcher);
    }

    void detachClause(Clause* clause) override {
        assert(clause->size() > 2);
        for (Lit lit : *clause) {
            auto it = std::find_if(watchers[~lit].begin(), watchers[~lit].end(), [clause](Watcher* w){ return w->cref == clause; });
            if (it != watchers[~lit].end()) {
                Watcher* watcher = *it;
                Lit lit0 = watcher->watch0;
                Lit lit1 = watcher->watch1;
                watchers[~lit0].erase(std::remove(watchers[~lit0].begin(), watchers[~lit0].end(), watcher), watchers[~lit0].end());
                watchers[~lit1].erase(std::remove(watchers[~lit1].begin(), watchers[~lit1].end(), watcher), watchers[~lit1].end());
                break;
            }
        }
    }

    inline Reason propagate_binary_clauses(Lit p) {
        for (Lit other : clause_db.binaries[p]) {
            lbool val = trail.value(other);
            if (val == l_Undef) {
                trail.propagate(other, Reason(~p, other));
            }
            else if (val == l_False) {
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
    inline Reason propagate_watched_clauses(Lit p) {
        std::vector<Watcher*>& list = watchers[p];

        auto keep = list.begin();
        for (auto iter = list.begin(); iter != list.end(); iter++) {
            Watcher* watcher = *iter;
            assert(watcher->watch0 == ~p || watcher->watch1 == ~p);
            Lit other = watcher->watch0 != ~p ? watcher->watch0 : watcher->watch1;
            lbool val = trail.value(other);
            if (val != l_True) { // Try to avoid inspecting the clause
                Clause* clause = watcher->cref;                
                for (Lit lit : *clause) {
                    if (lit != ~p && lit != other && trail.value(lit) != l_False) {
                        watcher->watch0 = lit;
                        watcher->watch1 = other;
                        watchers[~lit].push_back(watcher);
                        goto propagate_skip;
                    }
                }

                // did not find watch
                if (val == l_False) { // conflict
                    list.erase(keep, iter);
                    return Reason(clause);
                }
                else { // unit
                    trail.propagate(other, clause);
                }
            }
            *keep = *iter;
            keep++;
            propagate_skip:;
        }
        list.erase(keep, list.end());

        return Reason();
    }

    Reason propagate() override {
        Reason conflict;

        while (trail.qhead < trail.trail_size) {
            Lit p = trail[trail.qhead++];
            
            // Propagate binary clauses
            conflict = propagate_binary_clauses(p);
            if (conflict.exists()) return conflict;

            // Propagate other 2-watched clauses
            conflict = propagate_watched_clauses(p);
            if (conflict.exists()) return conflict;
        }

        return Reason();
    }
};

}

#endif