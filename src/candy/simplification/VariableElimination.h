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
    ClauseDatabase& clause_db;
    Trail& trail;

    std::vector<Lit> resolvent;

    const bool active;          // Perform variable elimination.
    const unsigned int clause_lim;     // Variables are not eliminated if it produces a resolvent with a length above this limit. 0 means no limit.

public:
    std::vector<Var> variables;
    std::vector<std::vector<Cl>> clauses;

    unsigned int nEliminated;

    unsigned int verbosity;

    VariableElimination(ClauseDatabase& clause_db_, Trail& trail_) : 
        clause_db(clause_db_), trail(trail_),
        resolvent(),
        active(VariableEliminationOptions::opt_use_elim),
        clause_lim(VariableEliminationOptions::opt_clause_lim), 
        variables(), clauses(), 
        nEliminated(0), verbosity(SolverOptions::verb)
    { }

    void init() { 
        clauses.resize(trail.nVars());
    }

    unsigned int nTouched() {
        return nEliminated; 
    }

    void eliminate(OccurenceList& occurences) {
        if (!active) return;
        nEliminated = 0;

        std::vector<Var> variables;
        for (unsigned int v = 0; v < trail.nVars(); v++) {
            if (trail.isDecisionVar(v)) variables.push_back(v);
        }

        std::sort(variables.begin(), variables.end(), [&occurences](Var v1, Var v2) { 
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

                eliminate(occurences, variable, pos, neg);

                if (clause_db.hasEmptyClause()) break;
            }
        }
        
        if (verbosity > 1) {
            std::cout << "c Eliminiated " << nEliminated << " variables" << std::endl;
        }
    }

    bool has_eliminated_variables() const {
        return variables.size() > 0;
    }

    bool is_eliminated(Var v) const {
        return clauses[v].size() > 0;
    }

    void set_eliminated(Var var, std::vector<Clause*> pos, std::vector<Clause*> neg) {
        variables.push_back(var);
        for (Clause* c : pos) clauses[var].push_back(Cl {c->begin(), c->end()} );
        for (Clause* c : neg) clauses[var].push_back(Cl {c->begin(), c->end()} );
        trail.setDecisionVar(var, false);
        nEliminated++;
    }

    std::vector<Cl> undo(Var var) {
        assert(is_eliminated(var));
        std::vector<Cl> correction_set;
        auto begin = std::find(variables.begin(), variables.end(), var);
        for (auto it = begin; it != variables.end(); it++) {
            correction_set.insert(correction_set.end(), clauses[*it].begin(), clauses[*it].end());
            clauses[*it].clear();
            trail.setDecisionVar(*it, true);
        }
        variables.erase(begin, variables.end());
        return correction_set;
    }

private:
    void eliminate(OccurenceList& occurences, Var variable, std::vector<Clause*> pos, std::vector<Clause*> neg) {
        assert(!is_eliminated(variable));

        if (pos.size() == 0 || neg.size() == 0) return;

        unsigned int nResolvents = 0;
        for (Clause* pc : pos) for (Clause* nc : neg) {
            unsigned int clause_size = 0;
            if (merge(*pc, *nc, variable, clause_size)) {
                if (clause_size == 0) {
                    clause_db.emptyClause();
                    return;
                }
                if (++nResolvents > pos.size() + neg.size() || (clause_lim > 0 && clause_size > clause_lim)) {
                    return; // out of bounds
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
        set_eliminated(variable, pos, neg);

        for (Clause* clause : pos) {
            clause_db.removeClause(clause);
        }
        for (Clause* clause : neg) {
            clause_db.removeClause(clause);
        }
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