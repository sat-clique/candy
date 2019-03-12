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

#include "candy/core/simplification/SubsumptionClauseDatabase.h"
#include "candy/core/simplification/SubsumptionClause.h"
#include "candy/core/Trail.h"
#include "candy/utils/Options.h"

namespace Candy {

class VariableElimination {
private:
    SubsumptionClauseDatabase& database;
    Trail& trail;

    std::vector<Lit> elimclauses;
    std::vector<char> eliminated;
    std::vector<char> frozen;

    std::vector<Lit> resolvent;

    const bool active;          // Perform variable elimination.

    const unsigned int clause_lim;     // Variables are not eliminated if it produces a resolvent with a length above this limit. 0 means no limit.

public:
    unsigned int nEliminated;

    VariableElimination(SubsumptionClauseDatabase& database_, Trail& trail_) : 
        database(database_),
        trail(trail_),
        elimclauses(), 
        eliminated(),
        frozen(),
        resolvent(),
        active(VariableEliminationOptions::opt_use_elim),
        clause_lim(VariableEliminationOptions::opt_clause_lim), 
        nEliminated(0)
    { }

    unsigned int nTouched() {
        return nEliminated; 
    }

    inline void grow(size_t size) {
        if (size > frozen.size()) {
            frozen.resize(size);
            eliminated.resize(size);
        }
    }

    inline void lock(Var v) {
        frozen[v] = true;
    }

    inline bool isEliminated(Var v) const {
        return eliminated[v];
    }

    bool eliminate() {
        nEliminated = 0;

        if (!active) {
            return true;
        }

        std::vector<Var> variables;
        for (unsigned int v = 0; v < frozen.size(); v++) {
            if (!frozen[v] && !isEliminated(v)) variables.push_back(v);
        }

        std::sort(variables.begin(), variables.end(), [this](Var v1, Var v2) { 
            return database.numOccurences(v1) > database.numOccurences(v2);
        });

        for (Var variable : variables) {
            if (!trail.defines(Lit(variable))) {
                std::vector<SubsumptionClause*> pos, neg; // split the occurrences into positive and negative
                for (SubsumptionClause* cl : database.refOccurences(variable)) {
                    if (!cl->is_deleted()) {
                        if (cl->contains(Lit(variable))) {
                            pos.push_back(cl);
                        } else {
                            assert(cl->contains(Lit(variable, true)));
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

    void extendModel(std::vector<lbool>& model) {
        Lit x;
        for (int i = elimclauses.size()-1, j; i > 0; i -= j) {
            for (j = elimclauses[i--]; j > 1; j--, i--) {
                Lit p = elimclauses[i]; 
                if ((model[p.var()] ^ p.sign()) != l_False)
                    goto next;
            }
            x = elimclauses[i];
            model[x.var()] = lbool(!x.sign());
        next: ;
        }
    }

private:

    bool eliminate(Var variable, std::vector<SubsumptionClause*> pos, std::vector<SubsumptionClause*> neg) {
        assert(!isEliminated(variable));
        
        size_t nResolvents = 0;
        for (SubsumptionClause* pc : pos) for (SubsumptionClause* nc : neg) {
            size_t clause_size = 0;
            if (!merge(*pc->get_clause(), *nc->get_clause(), variable, clause_size)) {
                continue; // resolvent is tautology
            }
            if (clause_size == 0) {
                return false; // resolved empty clause 
            }
            if (++nResolvents > pos.size() + neg.size() || (clause_lim > 0 && clause_size > clause_lim)) {
                return true;
            } 
        }
        
        if (pos.size() > neg.size()) {
            for (SubsumptionClause* c : neg) mkElimClause(variable, *c->get_clause());
            mkElimClause(Lit(variable));
        } else {
            for (SubsumptionClause* c : pos) mkElimClause(variable, *c->get_clause());
            mkElimClause(~Lit(variable));
        } 
        
        for (SubsumptionClause* pc : pos) for (SubsumptionClause* nc : neg) {
            if (merge(*pc->get_clause(), *nc->get_clause(), variable, resolvent)) {
                uint16_t lbd = std::min({ pc->lbd(), nc->lbd(), (uint16_t)(resolvent.size()-1) });
                database.create(resolvent.begin(), resolvent.end(), lbd);
                assert(resolvent.size() > 0);
            }
        }

        for (SubsumptionClause* clause : pos) database.remove(clause);
        for (SubsumptionClause* clause : neg) database.remove(clause);

        eliminated[variable] = true;
        nEliminated++;

        return true;
    }

    void mkElimClause(Lit x) {
        elimclauses.push_back(x);
        elimclauses.push_back((Lit)1);
    }

    void mkElimClause(Var v, const Clause& c) { 
        assert(c.contains(Lit(v)) || c.contains(Lit(v, true)));
        uint32_t first = elimclauses.size();
        
        // Copy clause to elimclauses-vector
        for (Lit lit : c) {
            if (lit.var() != v || first == elimclauses.size()) {
                elimclauses.push_back(lit);
            } else {
                // Swap such that 'v' will occur first in the clause:
                elimclauses.push_back(elimclauses[first]);
                elimclauses[first] = lit;
            }
        }
        
        // Store the length of the clause last:
        elimclauses.push_back((Lit)c.size());
    }

    // Returns FALSE if clause is always satisfied ('out_clause' should not be used).
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

    // Returns FALSE if clause is always satisfied.
    bool merge(const Clause& _ps, const Clause& _qs, Var v, size_t& size) {
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