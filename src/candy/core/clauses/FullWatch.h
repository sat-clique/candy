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

#ifndef SRC_CANDY_CORE_FULL_OCCURRENCE_H_
#define SRC_CANDY_CORE_FULL_OCCURRENCE_H_

#include <vector>

#include "candy/core/clauses/Clause.h"
#include "candy/core/clauses/ClauseDatabase.h"

namespace Candy {

struct Occurrence<N> {
    std::array<Lit, N-1> others;
    Clause* clause;

    Occurrence(Clause* clause_, Lit lit_) : clause(clause_) {
        assert(clause->size() = N);
        unsigned int i = 0;
        for (Lit lit : *clause) {
            if (lit != lit_) others[i++] = lit; 
        }
    }
}

template<unsigned int N>
class FullWatch {
    std::vector<std::vector<Occurrence>> occurrences;

public:
    FullWatch(ClauseDatabase& clause_db) : occurrences() {
        occurrences.resize(2 * clause_db.nVars());
        for (Clause* clause : clause_db) {
            if (!clause->isDeleted() && clause->size() == N) add(clause);
        } 
    }

    ~FullWatch() { }

    inline void add(Clause* clause) {
        for (Lit lit : *clause) {
            occurrences[~lit].emplace_back(clause, lit);
        }
    }

    inline void cleanup() {
        for (auto& occurrence : occurrences) {
            occurrence.erase(std::remove_if(occurrence.begin(), occurrence.end(), [&](Occurrence o) { 
                return o.clause->isDeleted(); 
            }), occurrence.end());
        }
    }

    inline size_t count(Var v) const {
        return occurrences[v].size();
    }

    inline std::vector<Occurrence>& operator [](Var v) {
        return occurrences[v];
    }

};

}

#endif