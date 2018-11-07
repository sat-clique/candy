/*
 * Propagate.h
 *
 *  Created on: Jul 18, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_PROPAGATE_H_
#define SRC_CANDY_CORE_PROPAGATE_H_

#include "candy/core/SolverTypes.h"
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

#define NWATCHES 2

class Propagate {
private:
    Trail& trail;

    std::array<OccLists<Lit, Watcher, WatcherDeleted>, 2> watches;

public:
    uint64_t nPropagations;

    Propagate(Trail& _trail) : trail(_trail), watches(), nPropagations(0) {
        for (auto& watchers : watches) {
            watchers = OccLists<Lit, Watcher, WatcherDeleted>();
        }
    }

    void init(size_t maxVars) {
        for (auto& watchers : watches) {
            watchers.init(mkLit(maxVars, true));
        }
    }

    std::vector<Watcher>& getBinaryWatchers(Lit p) {
        return watches[0][p];
    }

    void attachClause(Clause* cr) {
        assert(cr->size() > 1);
        uint_fast8_t pos = std::min(cr->size()-2, NWATCHES-1);
        watches[pos][~cr->first()].emplace_back(cr, cr->second());
        watches[pos][~cr->second()].emplace_back(cr, cr->first());
    }

    void detachClause(Clause* cr, bool strict = false) {
        assert(cr->size() > 1);
        uint_fast8_t pos = std::min(cr->size()-2, NWATCHES-1);
        if (strict) {
            std::vector<Watcher>& list0 = watches[pos][~cr->first()];
            std::vector<Watcher>& list1 = watches[pos][~cr->second()];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [cr](Watcher w){ return w.cref == cr; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [cr](Watcher w){ return w.cref == cr; }), list1.end());
        } else {
            watches[pos].smudge(~cr->first());
            watches[pos].smudge(~cr->second());
        }
    }

    void attachAll(std::vector<Clause*>& clauses) {
        for (Clause* clause : clauses) {
            attachClause(clause);
        }
    }

    void detachAll() {
        for (auto& watchers : watches) {
            watchers.clear();
        }
    }

    void cleanupWatchers() {
        for (auto& watchers : watches) {
            watchers.cleanAll();
        }
    }

    void sortWatchers() {
        size_t nVars = watches[0].size() / 2;
        for (size_t v = 0; v < nVars; v++) {
            Var vVar = checked_unsignedtosigned_cast<size_t, Var>(v);
            for (Lit l : { mkLit(vVar, false), mkLit(vVar, true) }) {
                sort(watches[1][l].begin(), watches[1][l].end(), [](Watcher w1, Watcher w2) {
                    Clause& c1 = *w1.cref;
                    Clause& c2 = *w2.cref;
                    return c1.size() < c2.size() || (c1.size() == c2.size() && c1.activity() > c2.activity());
                });
            }
        }
    }

    inline Clause* propagate_binary_clauses(Lit p) {
        std::vector<Watcher>& list = watches[0][p];
        for (Watcher& watcher : list) {
            lbool val = trail.value(watcher.blocker);
            if (val == l_False) {
                return watcher.cref;
            }
            if (val == l_Undef) {
                trail.uncheckedEnqueue(watcher.blocker, watcher.cref);
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
        std::vector<Watcher>& list = watches[1][p];

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
                            watches[1][~clause->second()].emplace_back(clause, clause->first());
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
