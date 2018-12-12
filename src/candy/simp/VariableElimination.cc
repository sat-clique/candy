#include "candy/simp/VariableElimination.h" 
#include "candy/utils/Options.h"


namespace Candy {

VariableElimination::VariableElimination(ClauseDatabase& clause_db_) : 
    clause_db(clause_db_),
    elimclauses(), 
    eliminated(),
    clause_lim(VariableEliminationOptions::opt_clause_lim), 
    grow(VariableEliminationOptions::opt_grow),
    resolvents(),
    resolved() { 

}

void VariableElimination::mkElimClause(std::vector<uint32_t>& elimclauses, Lit x) {
    elimclauses.push_back(toInt(x));
    elimclauses.push_back(1);
}

void VariableElimination::mkElimClause(std::vector<uint32_t>& elimclauses, Var v, const Clause& c) { 
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
bool VariableElimination::merge(const Clause& _ps, const Clause& _qs, Var v, std::vector<Lit>& out_clause) {
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
bool VariableElimination::merge(const Clause& _ps, const Clause& _qs, Var v, size_t& size) {
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

/**
 * Return true if v was eliminated, else false
 */
bool VariableElimination::eliminateVar(Var v) {
    assert(!isEliminated(v));

    resolvents.clear();
    resolved.clear();

    const std::vector<Clause*>& occurences = clause_db.refOccurences(v);

    // split the occurrences into positive and negative:
    std::vector<Clause*> pos, neg;
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
        if (merge(*pc, *nc, v, clause_size) && (++cnt > occurences.size() + grow || (clause_lim > 0 && clause_size > clause_lim))) {
            return false;
        } 
    }
    
    if (pos.size() > neg.size()) {
        for (Clause* c : neg) mkElimClause(elimclauses, v, *c);
        mkElimClause(elimclauses, mkLit(v));
    } else {
        for (Clause* c : pos) mkElimClause(elimclauses, v, *c);
        mkElimClause(elimclauses, ~mkLit(v));
    }

    // flag v as eliminated
    setEliminated(v);
    
    // produce clauses in cross product
    static std::vector<Lit> resolvent;
    resolvent.clear();
    for (Clause* pc : pos) for (Clause* nc : neg) {
        if (merge(*pc, *nc, v, resolvent)) {
            resolvents.emplace_back(resolvent.begin(), resolvent.end());
        }
    }

    resolved.insert(resolved.end(), occurences.begin(), occurences.end());

    return true;
}

void VariableElimination::extendModel(std::vector<lbool>& model) {
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

bool VariableElimination::isEliminated(Var v) const {
    if (eliminated.size() < v+1) {
        return false;
    }
    return eliminated[v];
}

void VariableElimination::setEliminated(Var v) {
    if (eliminated.size() < v+1) {
        eliminated.resize(v+1, (char)false);
    }
    eliminated[v] = true;
}

}