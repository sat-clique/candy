/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef SRC_CANDY_CORE_PROPAGATION2WLS1WOPT_H_
#define SRC_CANDY_CORE_PROPAGATION2WLS1WOPT_H_

#include "candy/core/SolverTypes.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/Clause.h"
#include "candy/core/Trail.h"
#include "candy/core/systems/PropagationInterface.h"
#include <array>
#include "candy/mtl/EMA.h"

namespace Candy {

// struct Watcher {
//     Clause* cref;
//     Lit blocker;

//     Watcher(Clause* cr, Lit p)
//      : cref(cr), blocker(p) { }
// };

class Propagation2WLStable1WOpt : public PropagationInterface {
private:
    ClauseDatabase& clause_db;
    Trail& trail;

    std::vector<std::vector<Watcher>> watchers;
    std::vector<std::vector<Clause*>> alert;

    unsigned int nDetached = 0;
    unsigned int nReattached = 0;
    unsigned int nAssumes = 0;
    unsigned int nMisses = 0;
    unsigned int nRollbacks = 0;

    double stability_factor;
    double dynamic_stability;
    double correction_value;

public:
    Propagation2WLStable1WOpt(ClauseDatabase& _clause_db, Trail& _trail) : 
        clause_db(_clause_db), trail(_trail), 
        watchers(2*clause_db.nVars()), 
        alert(2*clause_db.nVars()),
        stability_factor(1.0),
        dynamic_stability(Stability::opt_dynamic_stability),
        correction_value(0.0)
    {
        for (Clause* clause : clause_db) {
            if (clause->size() > 2) {
                attachClause(clause);
            } 
        }
    }

    void reset() override {
        double dAssume = pow((nMisses) / (nAssumes + 1.0) - dynamic_stability, 3);
        double dDetach = pow((nReattached + nRollbacks) / (nDetached + nRollbacks + 1.0) - dynamic_stability, 3);
        correction_value = (correction_value + dAssume + dDetach) / 3;
        stability_factor += correction_value;
        if (stability_factor < 0.5) stability_factor = 0.5;
        if (stability_factor > 1.0) stability_factor = 1.0;
        if (SolverOptions::verb > 2) {
            std::cout << "Detached/Reattached " << nDetached << " / " << nReattached << " Clauses " << std::endl;
            std::cout << "Assumed/Missed " << nAssumes << " / " << nMisses << " (Rollbacks " << nRollbacks << ")" << std::endl;
            std::cout << "Correction Value " << correction_value << std::endl;
        }
        nDetached = 0; nReattached = 0; nMisses = 0; nRollbacks = 0; nAssumes = 0;
        for (auto& w : watchers) w.clear();
        for (auto& w : alert) w.clear();
        for (Clause* clause : clause_db) {
            if (clause->size() > 2) {
                attachClause(clause);
            } 
        }
        if (SolverOptions::verb > 2) {
            std::cout << "Stability Factor " << stability_factor << std::endl;
            std::cout << "Detached Clauses " << nDetached << std::endl;
        }
    }

    inline double clause_stability(Clause* clause) {
        return (1 - trail.stability[~clause->first()] / (double)trail.nDecisions)
             * (1 - trail.stability[~clause->second()] / (double)trail.nDecisions);
    }

    void attachClause(Clause* clause) override {
        assert(clause->size() > 2);
        if (clause_stability(clause) > stability_factor) {
            if (alert[~clause->first()].size() == 0) nAssumes++;
            alert[~clause->first()].push_back(clause);
            nDetached++;
        }
        else {
            watchers[~clause->first()].emplace_back(clause, clause->second());            
            watchers[~clause->second()].emplace_back(clause, clause->first());
        }
    }

    void detachClause(Clause* clause) override {
        assert(clause->size() > 2);
        auto it = std::find(alert[~clause->first()].begin(), alert[~clause->first()].end(), clause);
        if (it != alert[~clause->first()].end()) {
            alert[~clause->first()].erase(it);
        }
        else {
            std::vector<Watcher>& list0 = watchers[~clause->first()];
            std::vector<Watcher>& list1 = watchers[~clause->second()];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [clause](Watcher w){ return w.cref == clause; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [clause](Watcher w){ return w.cref == clause; }), list1.end());
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
    Reason propagate_watched_clauses(Lit p) {
        std::vector<Watcher>& list = watchers[p];

        auto keep = list.begin();
        for (auto watcher = list.begin(); watcher != list.end(); watcher++) {
            lbool val = trail.value(watcher->blocker);

            if (val != l_True) { // Try to avoid inspecting the clause
                Clause* clause = watcher->cref;

                if (clause->isDeleted()) continue;

                if (clause->first() == ~p) { // Make sure the false literal is data[1]
                    clause->swap(0, 1);
                }

                if (watcher->blocker != clause->first()) {
                    watcher->blocker = clause->first(); 
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
                        return Reason(clause);
                    }
                    else { // unit
                        trail.propagate(clause->first(), clause);
                    }
                }
            }
            *keep = *watcher;
            keep++;
            propagate_skip:;
        }
        list.erase(keep, list.end());

        return Reason();
    }

    Reason propagate_alerts(Lit p) {
        if (alert[p].size() > 0) {
            Clause* comeback = nullptr;
            for (Clause* clause : alert[p]) {
                assert(clause->first() == ~p);

                unsigned int level = trail.level(clause->second());
                lbool value = trail.value(clause->second());

                if (value != l_True) {
                    clause->swap(0, 1); // now ~p is second
                    unsigned int w = (value == l_Undef) ? 1 : 0;
                    for (unsigned int pos = 2; pos < clause->size(); pos++) {
                        Lit lit = (*clause)[pos];
                        value = trail.value(lit);

                        if (value != l_False || trail.level(lit) > level) { 
                            clause->swap(w, pos);
                            level = trail.level(lit);
                            if (value == l_True) break;
                            if (value == l_Undef && ++w == 2) break;
                        }
                    }

                    if (value != l_True && w == 0) { // clause is false, need to fix trail later 
                        if (comeback == nullptr || level < trail.level(comeback->first())) {
                            comeback = clause;
                        }
                    }
                }

                watchers[~clause->first()].emplace_back(clause, clause->second());            
                watchers[~clause->second()].emplace_back(clause, clause->first());
            }
            nMisses++;
            nReattached += alert[p].size();
            alert[p].clear();
            if (comeback != nullptr) {
                unsigned int level = trail.level(comeback->first());
                if (level == trail.decisionLevel()) {
                    return Reason(comeback);
                }
                else { 
                    nRollbacks++;
                    trail.backtrack(level, false);
                    return Reason(Lit(p+1), Lit(p+1));
                }
            }
        }
        return Reason();
    }

    Reason propagate() override {
        Reason conflict;

        while (trail.qhead < trail.trail_size) {
            Lit p = trail[trail.qhead++];

            // Propagate alerts
            conflict = propagate_alerts(p);
            if (conflict.exists()) return conflict;
            
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
