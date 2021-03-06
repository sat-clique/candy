/*************************************************************************************************
Candy -- Copyright (c) 2020-2021, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef SRC_CANDY_CORE_NARIES_H_
#define SRC_CANDY_CORE_NARIES_H_

#include "candy/core/SolverTypes.h"

#include "candy/core/clauses/Clause.h"

namespace Candy {

template<unsigned int N>
struct Occurrence {
    Lit others[N-1];
    Clause* clause;

    Occurrence(Clause* clause_, Lit occ) : clause(clause_) { 
        unsigned int i = 0;
        for (Lit lit : *clause) {
            if (lit != occ) others[i++] = lit;
        }
        assert(i == N-1);
    }

    typedef Lit* iterator;

    inline iterator begin() {
        return others;
    }

    inline iterator end() {
        return others + N-1;
    }
    
};

template<unsigned int N>
class NaryClauses {
public:
    std::vector<std::vector<Occurrence<N>>> lists;

    NaryClauses(unsigned int nVars) : lists() {
        lists.resize(2*nVars);
    }

    void clear() {
        for (std::vector<Occurrence<N>>& occ : lists) occ.clear();
    }

    inline std::vector<Occurrence<N>>& operator [](Lit p) {
        return lists[p];
    }

    void add(Clause* clause) {
        assert(clause->size() == N);
        for (const Lit lit : *clause) {
            lists[~lit].emplace_back(clause, lit);
        }
    }

    void remove(Clause* clause) {
        assert(clause->size() == N);
        for (const Lit lit : *clause) {
            lists[~lit].erase(std::find_if(lists[~lit].begin(), lists[~lit].end(), [clause](Occurrence<N> o) { return o.clause == clause; }));
        }
    }

};

}

#endif