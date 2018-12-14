#ifndef _VARIABLE_ELIMINATION_H
#define _VARIABLE_ELIMINATION_H

#include <vector> 

#include "candy/core/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/core/Clause.h"
#include "candy/utils/Options.h"
#include "candy/core/Certificate.h"

namespace Candy {
  
template <class TPropagate> class VariableElimination {
private:
    ClauseDatabase& clause_db;
    Trail& trail;
    TPropagate& propagator;
    Certificate& certificate;

    std::vector<uint32_t> elimclauses;
    std::vector<char> eliminated;
    std::vector<char> frozen;

    const bool use_asymm;         // Shrink clauses by asymmetric branching.
    const bool use_elim;          // Perform variable elimination.
    const uint_fast8_t clause_lim;     // Variables are not eliminated if it produces a resolvent with a length above this limit. 0 means no limit.
    const uint_fast8_t grow_value;     // Allow a variable elimination step to grow by a number of clauses (default to zero).

public:
    VariableElimination(ClauseDatabase& clause_db_, Trail& trail_, TPropagate& propagator_, Certificate& certificate_) : 
        clause_db(clause_db_),
        trail(trail_),
        propagator(propagator_),
        certificate(certificate_),
        frozen(),
        elimclauses(), 
        eliminated(),
        use_asymm(VariableEliminationOptions::opt_use_asymm),
        use_elim(VariableEliminationOptions::opt_use_elim),
        clause_lim(VariableEliminationOptions::opt_clause_lim), 
        grow_value(VariableEliminationOptions::opt_grow)
    { }

    inline void grow(size_t size) {
        if (size > frozen.size()) {
            frozen.resize(size);
            eliminated.resize(size);
        }
    }

    inline void lock(Var v) {
        frozen[v] = true;
    }

    inline void setEliminated(Var v) {
        eliminated[v] = true;
    }

    inline bool isEliminated(Var v) const {
        return eliminated[v];
    }

    bool eliminate() {
        std::vector<Var> variables;
        for (unsigned int v = 0; v < frozen.size(); v++) {
            if (!frozen[v] && !isEliminated(v)) variables.push_back(v);
        }

        std::sort(variables.begin(), variables.end(), [this](Var v1, Var v2) { 
            return this->clause_db.numOccurences(v1) > this->clause_db.numOccurences(v2);
        });

        for (Var v : variables) {
            if (trail.isAssigned(v)) continue;

            if (use_asymm) {
                if (!asymmVar(v)) {
                    return false;
                }
            }

            if (trail.isAssigned(v)) continue;

            if (use_elim) {
                if (!eliminateVar(v)) {
                    return false;
                }
            }
        }

        return true;
    }

    bool asymmVar(Var v) {
        assert(trail.decisionLevel() == 0);

        const std::vector<Clause*> occurences = clause_db.copyOccurences(v);
        for (const Clause* clause : occurences) {
            if (!clause->isDeleted() && !trail.satisfies(*clause)) {
                trail.newDecisionLevel();
                Lit l = lit_Undef;
                for (Lit lit : *clause) {
                    if (var(lit) != v && !trail.isAssigned(var(lit))) {
                        trail.uncheckedEnqueue(~lit);
                    } else {
                        l = lit;
                    }
                }
                bool asymm = propagator.propagate() != nullptr;
                trail.cancelUntil(0);
                assert(l != lit_Undef);
                if (asymm) { // strengthen:
                    propagator.detachClause(clause);
                    clause_db.removeClause((Clause*)clause);

                    std::vector<Lit> lits = clause->except(l);
                    certificate.added(lits.begin(), lits.end());
                    certificate.removed(clause->begin(), clause->end());
                    
                    if (lits.size() == 0) {
                        return false;
                    }
                    else if (lits.size() == 1) {
                        return trail.newFact(lits.front()) && propagator.propagate() == nullptr;
                    }
                    else {
                        Clause* new_clause = clause_db.createClause(lits.begin(), lits.end(), clause->getLBD());
                        propagator.attachClause(new_clause);
                    } 
                }
            }
        }
        return true;
    }

    bool eliminateVar(Var v) {
        assert(!isEliminated(v));

        const std::vector<Clause*> occurences = clause_db.copyOccurences(v);
        std::vector<Clause*> pos, neg; // split the occurrences into positive and negative
        for (Clause* cl : occurences) {
            if (cl->contains(mkLit(v))) {
                pos.push_back(cl);
            } else {
                neg.push_back(cl);
            }
        }
        
        // increase in number of clauses must stay within the allowed ('grow') and no clause must exceed the limit on the maximal clause size:
        size_t cnt = 0;
        for (Clause* pc : pos) for (Clause* nc : neg) {
            size_t clause_size = 0;
            if (merge(*pc, *nc, v, clause_size) && (++cnt > occurences.size() + grow_value || (clause_lim > 0 && clause_size > clause_lim))) {
                return true;
            } 
        }
        
        if (pos.size() > neg.size()) {
            for (Clause* c : neg) mkElimClause(elimclauses, v, *c);
            mkElimClause(elimclauses, mkLit(v));
        } else {
            for (Clause* c : pos) mkElimClause(elimclauses, v, *c);
            mkElimClause(elimclauses, ~mkLit(v));
        } 

        setEliminated(v);
        
        static std::vector<Lit> resolvent;
        resolvent.clear();
        for (Clause* pc : pos) for (Clause* nc : neg) {
            if (merge(*pc, *nc, v, resolvent)) {
                certificate.added(resolvent.begin(), resolvent.end());
                if (resolvent.size() == 0) {
                    return false;
                }
                else if (resolvent.size() == 1) {
                    return trail.newFact(resolvent.front()) && propagator.propagate() == nullptr;
                }
                else {
                    unsigned int lbd = std::min(pc->getLBD(), nc->getLBD());
                    Clause* clause = clause_db.createClause(resolvent.begin(), resolvent.end(), lbd);
                    propagator.attachClause(clause);
                }
            }
        }

        for (const Clause* c : occurences) {
            certificate.removed(c->begin(), c->end());
            propagator.detachClause(c);
            clause_db.removeClause((Clause*)c);
        }

        return true;
    }

    void extendModel(std::vector<lbool>& model) {
        Lit x;
        for (int i = elimclauses.size()-1, j; i > 0; i -= j) {
            for (j = elimclauses[i--]; j > 1; j--, i--) {
                Lit p = toLit(elimclauses[i]); 
                if ((model[var(p)] ^ sign(p)) != l_False)
                    goto next;
            }
            x = toLit(elimclauses[i]);
            model[var(x)] = lbool(!sign(x));
        next: ;
        }
    }

private:
    void mkElimClause(std::vector<uint32_t>& elimclauses, Lit x) {
        elimclauses.push_back(toInt(x));
        elimclauses.push_back(1);
    }
    void mkElimClause(std::vector<uint32_t>& elimclauses, Var v, const Clause& c) { 
        assert(c.contains(v));
        uint32_t first = elimclauses.size();
        
        // Copy clause to elimclauses-vector
        for (Lit lit : c) {
            if (var(lit) != v || first == elimclauses.size()) {
                elimclauses.push_back(lit);
            } else {
                // Swap such that 'v' will occur first in the clause:
                elimclauses.push_back(elimclauses[first]);
                elimclauses[first] = lit;
            }
        }
        
        // Store the length of the clause last:
        elimclauses.push_back(c.size());
    }

    // Returns FALSE if clause is always satisfied ('out_clause' should not be used).
    bool merge(const Clause& _ps, const Clause& _qs, Var v, std::vector<Lit>& out_clause) {
        assert(_ps.contains(v));
        assert(_qs.contains(v));
        out_clause.clear();
        
        bool ps_smallest = _ps.size() < _qs.size();
        const Clause& ps = ps_smallest ? _qs : _ps;
        const Clause& qs = ps_smallest ? _ps : _qs;
        
        for (Lit qlit : qs) {
            if (var(qlit) != v) {
                auto p = std::find_if(ps.begin(), ps.end(), [qlit] (Lit plit) { return var(plit) == var(qlit); });
                if (p == ps.end()) {
                    out_clause.push_back(qlit);
                }
                else if (*p == ~qlit) {
                    return false;
                }
            }
        }
        
        for (Lit plit : ps) {
            if (var(plit) != v) {
                out_clause.push_back(plit);
            }
        }
        
        return true;
    }

    // Returns FALSE if clause is always satisfied.
    bool merge(const Clause& _ps, const Clause& _qs, Var v, size_t& size) {
        assert(_ps.contains(v));
        assert(_qs.contains(v));
        
        bool ps_smallest = _ps.size() < _qs.size();
        const Clause& ps = ps_smallest ? _qs : _ps;
        const Clause& qs = ps_smallest ? _ps : _qs;
        
        size = ps.size() - 1;
        
        for (Lit qlit : qs) {
            if (var(qlit) != v) {
                auto p = std::find_if(ps.begin(), ps.end(), [qlit] (Lit plit) { return var(plit) == var(qlit); });
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