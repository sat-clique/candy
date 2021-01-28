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

#ifndef _VARIABLE_ELIMINATION_H
#define _VARIABLE_ELIMINATION_H

#include <vector> 

#include "candy/simplification/OccurenceList.h"
#include "candy/core/Trail.h"
#include "candy/core/CandySolverResult.h"
#include "candy/utils/Options.h"

namespace Candy {

class VariableElimination {
private:
    OccurenceList& occurences;
    ClauseDatabase& clause_db;
    Trail& trail;

    std::vector<char> frozen;

    std::vector<Lit> resolvent;

    const bool active;          // Perform variable elimination.

    const unsigned int clause_lim;     // Variables are not eliminated if it produces a resolvent with a length above this limit. 0 means no limit.

public:
    unsigned int nEliminated;

    VariableElimination(OccurenceList& occurences_, ClauseDatabase& clause_db_, Trail& trail_) : 
        occurences(occurences_),
        clause_db(clause_db_),
        trail(trail_),
        frozen(),
        resolvent(),
        active(VariableEliminationOptions::opt_use_elim),
        clause_lim(VariableEliminationOptions::opt_clause_lim), 
        nEliminated(0)
    { 
        frozen.resize(trail.nVars());
        for (Lit lit : trail.assumptions) {
            frozen[lit.var()] = true;
        }
    }

    unsigned int nTouched() {
        return nEliminated; 
    }

    bool eliminate() {
        if (!active) return true;
        nEliminated = 0;

        std::vector<Var> variables;
        for (unsigned int v = 0; v < frozen.size(); v++) {
            if (!frozen[v] && !clause_db.eliminated.is_eliminated(v)) variables.push_back(v);
        }

        std::sort(variables.begin(), variables.end(), [this](Var v1, Var v2) { 
            return occurences.count(v1) > occurences.count(v2);
        });

        std::vector<Clause*> pos, neg; // split occurrences by polarity
        for (Var variable : variables) {
            if (trail.value(variable) == l_Undef) {
                pos.clear(); neg.clear();
                for (Clause* cl : occurences[variable]) {
                    if (!cl->isDeleted()) {
                        if (cl->contains(Lit(variable))) {
                            pos.push_back(cl);
                        } else {
                            neg.push_back(cl);
                        }
                    }
                }

                if (!eliminate(variable, pos, neg)) { 
                    return false;
                }
            }
        }

        return true;
    }

private:
    bool eliminate(Var variable, std::vector<Clause*> pos, std::vector<Clause*> neg) {
        assert(!clause_db.eliminated.is_eliminated(variable));

        if (pos.size() == 0 || neg.size() == 0) {
            return true;
        }

        unsigned int nResolvents = 0;
        for (Clause* pc : pos) for (Clause* nc : neg) {
            unsigned int clause_size = 0;
            if (merge(*pc, *nc, variable, clause_size)) {
                if (clause_size == 0) {
                    return false; // resolved empty clause 
                }
                if (++nResolvents > pos.size() + neg.size() || (clause_lim > 0 && clause_size > clause_lim)) {
                    return true; // out of bounds
                }
            } 
        }
        
        for (Clause* pc : pos) for (Clause* nc : neg) {
            if (merge(*pc, *nc, variable, resolvent)) {
                uint16_t lbd = std::min({ (uint16_t)pc->getLBD(), (uint16_t)nc->getLBD(), (uint16_t)(resolvent.size()-1) });
                // std::cout << "c Creating resolvent " << resolvent << std::endl;
                Clause* clause = clause_db.createClause(resolvent.begin(), resolvent.end(), lbd);
                occurences.add(clause);
            }
        }

        // std::cout << "Eliminated Variable " << variable << std::endl;
        clause_db.eliminated.set_eliminated(variable, pos, neg);

        for (Clause* clause : pos) {
            clause_db.removeClause(clause);
        }
        for (Clause* clause : neg) {
            clause_db.removeClause(clause);
        }

        nEliminated++;        
        trail.setDecisionVar(variable, false);

        return true;
    }

    // returns FALSE iff resolvent is tautologic ('out_clause' should not be used).
    bool merge(const Clause& _ps, const Clause& _qs, Var v, std::vector<Lit>& out_clause) {
        assert(_ps.contains(Lit(v)));
        assert(_qs.contains(Lit(v, true)));
        out_clause.clear();
        
        bool ps_smallest = _ps.size() < _qs.size();
        const Clause& ps = ps_smallest ? _qs : _ps;
        const Clause& qs = ps_smallest ? _ps : _qs;
        
        for (Lit qlit : qs) {
            if (qlit.var() != v) {
                auto p = std::find_if(ps.begin(), ps.end(), [qlit] (Lit plit) { return plit.var() == qlit.var(); });
                if (p == ps.end()) {
                    out_clause.push_back(qlit);
                }
                else if (*p == ~qlit) {
                    return false;
                }
            }
        }
        
        for (Lit plit : ps) {
            if (plit.var() != v) {
                out_clause.push_back(plit);
            }
        }
        
        return true;
    }

    // returns FALSE iff resolvent is tautologic
    bool merge(const Clause& _ps, const Clause& _qs, Var v, unsigned int& size) {
        assert(_ps.contains(Lit(v)));
        assert(_qs.contains(Lit(v, true)));
        
        bool ps_smallest = _ps.size() < _qs.size();
        const Clause& ps = ps_smallest ? _qs : _ps;
        const Clause& qs = ps_smallest ? _ps : _qs;
        
        size = ps.size() - 1;
        
        for (Lit qlit : qs) {
            if (qlit.var() != v) {
                auto p = std::find_if(ps.begin(), ps.end(), [qlit] (Lit plit) { return plit.var() == qlit.var(); });
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

};

}

#endif