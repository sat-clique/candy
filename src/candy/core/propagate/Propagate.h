/*
 * Propagate.h
 *
 *  Created on: Jul 18, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_PROPAGATE_H_
#define SRC_CANDY_CORE_PROPAGATE_H_

#include "candy/core/SolverTypes.h"
#include "candy/core/ClauseDatabase.h"
#include "candy/core/Clause.h"
#include "candy/core/Trail.h"
#include "candy/utils/CheckedCast.h"

#include <array>

//#define FUTURE_PROPAGATE

namespace Candy {

// Helper structures:
struct Watcher {
    Clause* cref;
    Lit blocker;
    Watcher() :
        cref(nullptr), blocker(lit_Undef) {}
    Watcher(Clause* cr, Lit p) :
        cref(cr), blocker(p) {}
    bool operator==(const Watcher& w) const {
        return cref == w.cref;
    }
    bool operator!=(const Watcher& w) const {
        return cref != w.cref;
    }
    bool isDeleted() {
        return cref->isDeleted() == 1;
    }
};

struct WatcherDeleted {
    WatcherDeleted() { }
    inline bool operator()(const Watcher& w) const {
        return w.cref->isDeleted() == 1;
    }
};

class Propagate {
private:
    ClauseDatabase& clause_db;
    Trail& trail;

    OccLists<Lit, Watcher, WatcherDeleted> watchers;

public:
    uint64_t nPropagations;

    Propagate(ClauseDatabase& _clause_db, Trail& _trail)
        : clause_db(_clause_db), trail(_trail), nPropagations(0) {
        watchers = OccLists<Lit, Watcher, WatcherDeleted>();
    }

    void init(size_t maxVars) {
        watchers.init(mkLit(maxVars, true));
    }

    void attachClause(Clause* clause) {
        assert(clause->size() > 1);
        if (clause->size() > 2) {
            watchers[~clause->first()].emplace_back(clause, clause->second());
            watchers[~clause->second()].emplace_back(clause, clause->first());
        }
    }

    void detachClause(const Clause* clause) {
        assert(clause->size() > 1);
        if (clause->size() > 2) {
            std::vector<Watcher>& list0 = watchers[~clause->first()];
            std::vector<Watcher>& list1 = watchers[~clause->second()];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [clause](Watcher w){ return w.cref == clause; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [clause](Watcher w){ return w.cref == clause; }), list1.end());
        }
    }

    void attachAll() {
        for (Clause* clause : clause_db) {
            attachClause(clause);
        }
    }

    void detachAll() {
        watchers.clear();
    }

    void sortWatchers() {
        size_t nVars = watchers.size() / 2;
        for (size_t v = 0; v < nVars; v++) {
            Var vVar = checked_unsignedtosigned_cast<size_t, Var>(v);
            for (Lit l : { mkLit(vVar, false), mkLit(vVar, true) }) {
                sort(watchers[l].begin(), watchers[l].end(), [](Watcher w1, Watcher w2) {
                    return w1.cref->size() < w2.cref->size();
                });
            }
        }
    }

    inline Clause* propagate_binary_clauses(Lit p) {
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
    inline Clause* propagate_watched_clauses(Lit p) {
        std::vector<Watcher>& list = watchers[p];

        auto keep = list.begin();
        for (auto watcher = list.begin(); watcher != list.end(); watcher++) {
            lbool val = trail.value(watcher->blocker);
            if (val != l_True) { // Try to avoid inspecting the clause
                Clause* clause = watcher->cref;

                if (clause->first() == ~p) { // Make sure the false literal is data[1]
                    clause->swap(0, 1);
                }

                if (watcher->blocker != clause->first()) {
                    watcher->blocker = clause->first(); // repair blocker (why?)
                    val = trail.value(clause->first());
                }

                if (val != l_True) {
                    for (uint_fast16_t k = 2; k < clause->size(); k++) {
                        if (trail.value((*clause)[k]) != l_False) {
                            clause->swap(1, k);
                            watchers[~clause->second()].emplace_back(clause, clause->first());
                            goto propagate_skip;
                        }
                    }

                    // did not find watch
                    if (val == l_False) { // conflict
                        list.erase(keep, watcher);
                        return clause;
                    }
                    else { // unit
                        trail.uncheckedEnqueue(clause->first(), clause);
                    }
                }
            }
            *keep = *watcher;
            keep++;
            propagate_skip:;
        }
        list.erase(keep, list.end());

        return nullptr;
    }

    Clause* propagate() {
        Clause* conflict = nullptr;
        unsigned int old_qhead = trail.qhead;

        while (trail.qhead < trail.trail_size) {
            Lit p = trail[trail.qhead++];
            
            // Propagate binary clauses
            conflict = propagate_binary_clauses(p);
            if (conflict != nullptr) break;

            // Propagate other 2-watched clauses
            conflict = propagate_watched_clauses(p);
            if (conflict != nullptr) break;
        }

        nPropagations += trail.qhead - old_qhead;
        return conflict;
    }
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_PROPAGATE_H_ */
