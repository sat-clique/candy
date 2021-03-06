/*************************************************************************************************
Candy -- Copyright (c) 2015-2021, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef SRC_CANDY_CORE_PROPAGATION2WLX_H_
#define SRC_CANDY_CORE_PROPAGATION2WLX_H_

#include "candy/core/SolverTypes.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/Clause.h"
#include "candy/core/clauses/NaryClauses.h"
#include "candy/core/Trail.h"
#include "candy/core/systems/PropagationInterface.h"

namespace Candy {

// struct Watcher {
//     Clause* cref;
//     Lit blocker;

//     Watcher(Clause* cr, Lit p)
//      : cref(cr), blocker(p) { }
// };

template<unsigned int X>
class PropagateX {
public:
    NaryClauses<X> nary;

    PropagateX(unsigned int nVars) : nary(nVars) {}

    inline Reason propagate_nary_clauses(Trail& trail, Lit p) {
        for (const Occurrence<X>& o : nary[p]) {
            Lit prop = lit_Undef;
            for (Lit lit : o.others) {
                lbool val = trail.value(lit);
                if (val == l_True) {
                    goto continue2;
                }
                else if (val == l_Undef) {
                    if (prop == lit_Undef) {
                        prop = lit;
                    } else {
                        goto continue2;
                    }
                }
            }
            if (prop == lit_Undef) {
                return Reason(o.clause);
            } 
            else {
                trail.propagate(prop, Reason(o.clause));
            }
            continue2:;
        }
        return Reason();
    }

    inline void clear() {
        nary.clear();
    }

    inline void attach(Clause* clause) {
        nary.add(clause);
    }

    inline void detach(Clause* clause) {
        nary.remove(clause);
    }
};

template<> class PropagateX<0> {
public:
    PropagateX(unsigned int nVars) {}
    inline Reason propagate_nary_clauses(Trail& trail, Lit p) { return Reason(); }
    inline void clear() {}
    inline void attach(Clause* clause) {}
    inline void detach(Clause* clause) {}
};

template<> class PropagateX<1> : public PropagateX<0> {
public:
    PropagateX(unsigned int nVars) : PropagateX<0>(nVars) {}
};

template<> class PropagateX<2> : public PropagateX<0> {
public:
    PropagateX(unsigned int nVars) : PropagateX<0>(nVars) {}
};

template<unsigned int X = 0, unsigned int Y = 1, unsigned int Z = 2>
class Propagation2WLX : public PropagationInterface, public PropagateX<X>, public PropagateX<Y>, public PropagateX<Z> {
    ClauseDatabase& clause_db;
    Trail& trail;

    std::vector<std::vector<Watcher>> watchers;

public:
    Propagation2WLX(ClauseDatabase& _clause_db, Trail& _trail) : 
        PropagateX<X>(_clause_db.nVars()), PropagateX<Y>(_clause_db.nVars()), PropagateX<Z>(_clause_db.nVars()), 
        clause_db(_clause_db), trail(_trail), watchers()
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
        PropagateX<X>::clear();
        PropagateX<Y>::clear();
        PropagateX<Z>::clear();
        for (Clause* clause : clause_db) {
            if (clause->size() > 2) {
                attachClause(clause);
            } 
        }
    }

    void attachClause(Clause* clause) override {
        assert(clause->size() > 2);

        if (clause->size() == X) {
            PropagateX<X>::attach(clause);
        } 
        else if (clause->size() == Y) {
            PropagateX<Y>::attach(clause);
        }
        else if (clause->size() == Z) {
            PropagateX<Z>::attach(clause);
        }
        else {
            watchers[~clause->first()].emplace_back(clause, clause->second());
            watchers[~clause->second()].emplace_back(clause, clause->first());
        }
    }

    void detachClause(Clause* clause) override {
        assert(clause->size() > 2);
        
        if (clause->size() == X) {
            PropagateX<X>::detach(clause);
        } 
        else if (clause->size() == Y) {
            PropagateX<Y>::detach(clause);
        }
        else if (clause->size() == Z) {
            PropagateX<Z>::detach(clause);
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

    Reason propagate() override {
        Reason conflict;

        while (trail.qhead < trail.trail_size) {
            Lit p = trail[trail.qhead++];
            
            // Propagate binary clauses
            conflict = propagate_binary_clauses(p);
            if (conflict.exists()) return conflict;
            
            // Propagate X-ary clauses
            conflict = PropagateX<X>::propagate_nary_clauses(trail, p);
            if (conflict.exists()) return conflict;

            // Propagate Y-ary clauses
            conflict = PropagateX<Y>::propagate_nary_clauses(trail, p);
            if (conflict.exists()) return conflict;

            // Propagate Z-ary clauses
            conflict = PropagateX<Z>::propagate_nary_clauses(trail, p);
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
