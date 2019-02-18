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

#ifndef SRC_CANDY_CORE_PROPAGATE_THREADSAFE_H_
#define SRC_CANDY_CORE_PROPAGATE_THREADSAFE_H_

#include "candy/core/SolverTypes.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/Clause.h"
#include "candy/core/Trail.h"
#include "candy/utils/CheckedCast.h"

#include <array>

namespace Candy {

struct WatcherTS {
    const Clause* cref;
    Lit watch0;
    Lit watch1;

    WatcherTS(const Clause* clause, Lit one, Lit two)
     : cref(clause), watch0(one), watch1(two) {}

    ~WatcherTS() {}
};

class StaticPropagate {
private:
    ClauseDatabase& clause_db;
    Trail& trail;

    std::vector<std::vector<WatcherTS*>> watchers;

public:
    StaticPropagate(ClauseDatabase& _clause_db, Trail& _trail)
        : clause_db(_clause_db), trail(_trail), watchers() { }

    ~StaticPropagate() {
        clear();
    }

    void init(size_t maxVars) {
        watchers.resize(mkLit(maxVars, true));
    }

    void attachClause(const Clause* clause) {
        assert(clause->size() > 2);
        WatcherTS* watcher = new WatcherTS(clause, clause->first(), clause->second());
        watchers[~(clause->first())].push_back(watcher);
        watchers[~(clause->second())].push_back(watcher);
    }

    void detachClause(const Clause* clause) {
        assert(clause->size() > 2);
        for (Lit lit : *clause) {
            auto it = std::find_if(watchers[~lit].begin(), watchers[~lit].end(), [clause](WatcherTS* w){ return w->cref == clause; });
            if (it != watchers[~lit].end()) {
                WatcherTS* watcher = *it;
                Lit lit0 = watcher->watch0;
                Lit lit1 = watcher->watch1;
                watchers[~lit0].erase(std::remove(watchers[~lit0].begin(), watchers[~lit0].end(), watcher), watchers[~lit0].end());
                watchers[~lit1].erase(std::remove(watchers[~lit1].begin(), watchers[~lit1].end(), watcher), watchers[~lit1].end());
                delete watcher;
                break;
            }
        }
    }

    void clear() {
        for (unsigned int lit = 0; lit < watchers.size(); lit++) {
            for (WatcherTS* watcher : watchers[lit]) {
                if (watcher->cref != nullptr) {
                    watcher->cref = nullptr;
                } else {
                    delete watcher;
                }
            }
            watchers[lit].clear();
        }
    }

    void sortWatchers() {
        size_t nVars = watchers.size() / 2;
        for (size_t v = 0; v < nVars; v++) {
            Var vVar = checked_unsignedtosigned_cast<size_t, Var>(v);
            for (Lit l : { mkLit(vVar, false), mkLit(vVar, true) }) {
                sort(watchers[l].begin(), watchers[l].end(), [](WatcherTS* w1, WatcherTS* w2) {
                    return w1->cref->size() < w2->cref->size();
                });
            }
        }
    }

    inline const Clause* propagate_binary_clauses(Lit p) {
        const std::vector<BinaryWatcher>& list = clause_db.getBinaryWatchers(p);
        for (BinaryWatcher watcher : list) {
            lbool val = trail.value(watcher.other);
            if (val == l_False) {
                return watcher.clause;
            }
            if (val == l_Undef) {
                trail.uncheckedEnqueue(watcher.other, watcher.clause);
            }
        }
        return nullptr;
    }

    /**************************************************************************************************
     *
     *  propagate : [void]  ->  [Clause*]
     *
     *  Description:
     *    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
     *    otherwise CRef_Undef.
     *
     *    Post-conditions:
     *      * the propagation queue is empty, even if there was a conflict.
     **************************************************************************************************/
    inline const Clause* propagate_watched_clauses(Lit p) {
        std::vector<WatcherTS*>& list = watchers[p];

        auto keep = list.begin();
        for (auto iter = list.begin(); iter != list.end(); iter++) {
            WatcherTS* watcher = *iter;
            assert(watcher->watch0 == ~p || watcher->watch1 == ~p);
            Lit other = watcher->watch0 != ~p ? watcher->watch0 : watcher->watch1;
            lbool val = trail.value(other);
            if (val != l_True) { // Try to avoid inspecting the clause
                const Clause* clause = watcher->cref;                
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
                    return clause;
                }
                else { // unit
                    trail.uncheckedEnqueue(other, (Clause*)clause);
                }
            }
            *keep = *iter;
            keep++;
            propagate_skip:;
        }
        list.erase(keep, list.end());

        return nullptr;
    }

    const Clause* propagate() {
        const Clause* conflict = nullptr;

        while (trail.qhead < trail.trail_size) {
            Lit p = trail[trail.qhead++];
            
            // Propagate binary clauses
            conflict = propagate_binary_clauses(p);
            if (conflict != nullptr) break;

            // Propagate other 2-watched clauses
            conflict = propagate_watched_clauses(p);
            if (conflict != nullptr) break;
        }

        return conflict;
    }
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_PROPAGATE_H_ */
