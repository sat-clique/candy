/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

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
 **************************************************************************************************/

#ifndef SRC_CANDY_CORE_SUBSUMPTION_CLAUSE_DATABASE_H_
#define SRC_CANDY_CORE_SUBSUMPTION_CLAUSE_DATABASE_H_

#include <vector>

#include "candy/core/clauses/Clause.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/Certificate.h"
#include "candy/core/Trail.h"
#include "candy/mtl/Memory.h"
#include "candy/frontend/CLIOptions.h"

namespace Candy {

class SubsumptionClauseDatabase {
private:
    ClauseDatabase& clause_db;

    Memory<SubsumptionClause> subsumption_clauses;
    std::vector<std::vector<SubsumptionClause*>> occurrences;

    inline void addToOccurenceLists(SubsumptionClause* subsumption_clause) {
        for (Lit lit : *subsumption_clause->get_clause()) {
            occurrences[lit.var()].push_back(subsumption_clause);
        }
    }

public:
    SubsumptionClauseDatabase(ClauseDatabase& clause_db)
     : clause_db(clause_db), subsumption_clauses(), occurrences()
    { }

    ~SubsumptionClauseDatabase() { }

    inline void init() {
        if (clause_db.nVars() > occurrences.size()) {
            occurrences.resize(clause_db.nVars());
        }
        for (Clause* clause : clause_db) {
            SubsumptionClause* subsumption_clause = new (subsumption_clauses.allocate()) SubsumptionClause(clause);
            addToOccurenceLists(subsumption_clause);
        }
    }

    inline void finalize() {
        subsumption_clauses.free_all();
        occurrences.clear();
    }

    inline void cleanup() {
        for (auto& occurrence : occurrences) {
            occurrence.erase(std::remove_if(occurrence.begin(), occurrence.end(), [&](SubsumptionClause* clause) { 
                return clause->is_deleted(); 
            }), occurrence.end());
        }
    }

    inline size_t numOccurences(Var v) {
        return occurrences[v].size();
    }

    inline std::vector<SubsumptionClause*> copyOccurences(Var v) {
        return std::vector<SubsumptionClause*>(occurrences[v].begin(), occurrences[v].end());
    }

    inline std::vector<SubsumptionClause*>& refOccurences(Var v) {
        return occurrences[v];
    }

    typedef Memory<SubsumptionClause>::const_iterator const_iterator;

    inline const_iterator begin() const {
        return subsumption_clauses.begin();
    }

    inline const_iterator end() const {
        return subsumption_clauses.end();
    }

    template<typename Iterator>
    inline SubsumptionClause* create(Iterator begin, Iterator end, unsigned int lbd = 0) {
        assert(std::distance(begin, end) > 0);
        Clause* clause = clause_db.createClause(begin, end, lbd);
        SubsumptionClause* subsumption_clause = new (subsumption_clauses.allocate()) SubsumptionClause(clause);
        addToOccurenceLists(subsumption_clause);        
        return subsumption_clause;
    }

    inline void remove(SubsumptionClause* subsumption_clause) {
        clause_db.removeClause(subsumption_clause->get_clause());
        subsumption_clause->set_deleted();
    }

    inline void strengthen(SubsumptionClause* subsumption_clause, Lit lit) {
        assert(subsumption_clause->size() > 1);
        Clause* new_clause = clause_db.strengthenClause(subsumption_clause->get_clause(), lit);
        subsumption_clause->reset(new_clause);
        occurrences[lit.var()].erase(std::remove(occurrences[lit.var()].begin(), occurrences[lit.var()].end(), subsumption_clause), occurrences[lit.var()].end());
    }

};

}

#endif