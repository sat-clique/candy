#ifndef _VARIABLE_ELIMINATION_H
#define _VARIABLE_ELIMINATION_H

#include <vector> 

#include "candy/core/ClauseDatabase.h"
#include "candy/core/Clause.h"
#include "candy/utils/Options.h"

namespace Candy {

namespace VariableEliminationOptions {
    extern Glucose::IntOption opt_clause_lim;
    extern Glucose::IntOption opt_grow; 
}
  
class VariableElimination {
public:

    std::vector<Cl> resolvents;
    std::vector<Clause*> resolved;

    VariableElimination(ClauseDatabase& clause_db_);

    bool eliminateVar(Var v);

    void extendModel(std::vector<lbool>& model);

    bool isEliminated(Var v) const;

private:

    ClauseDatabase& clause_db;

    std::vector<uint32_t> elimclauses;
    std::vector<char> eliminated;

    uint8_t clause_lim;        // Variables are not eliminated if it produces a resolvent with a length above this limit. 0 means no limit.
    uint8_t grow;              // Allow a variable elimination step to grow by a number of clauses (default to zero).

    void mkElimClause(std::vector<uint32_t>& elimclauses, Lit x);
    void mkElimClause(std::vector<uint32_t>& elimclauses, Var v, const Clause& c);

    bool merge(const Clause& _ps, const Clause& _qs, Var v, std::vector<Lit>& out_clause);
    bool merge(const Clause& _ps, const Clause& _qs, Var v, size_t& size);

    void setEliminated(Var v);

};

}

#endif