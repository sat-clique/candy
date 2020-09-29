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
#include "candy/utils/CLIOptions.h"

namespace Candy {

class OccurenceList {
private:
    std::vector<std::vector<Clause*>> occurrences;

public:
    OccurenceList(ClauseDatabase& clause_db) : occurrences() {
        occurrences.resize(clause_db.nVars());
        for (Clause* clause : clause_db) {
            if (!clause->isDeleted()) add(clause);
        } 
    }

    ~OccurenceList() { }

    inline void add(Clause* clause) {
        for (Lit lit : *clause) {
            occurrences[lit.var()].push_back(clause);
        }
    }

    inline void cleanup() {
        for (auto& occurrence : occurrences) {
            occurrence.erase(std::remove_if(occurrence.begin(), occurrence.end(), [&](Clause* clause) { 
                return clause->isDeleted(); 
            }), occurrence.end());
        }
    }

    inline size_t count(Var v) {
        return occurrences[v].size();
    }

    inline std::vector<Clause*> copy(Var v) {
        return std::vector<Clause*>(occurrences[v].begin(), occurrences[v].end());
    }

    inline std::vector<Clause*>& operator [](Var v) {
        return occurrences[v];
    }

};

}

#endif