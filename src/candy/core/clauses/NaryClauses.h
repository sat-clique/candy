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
    Lit[N-1] others;
    Clause* clause;

    Occurrence(Clause* clause_, Lit occ) : clause(clause_) { 
        assert(clause_.size() == N);
        unsigned int i = 0;
        for (Lit lit : clause) {
            if (lit != occ) others[i++] = lit;
        }
        assert(i == N);
    }

    typedef const Lit* const_iterator;

    inline const_iterator begin() const {
        return others;
    }

    inline const_iterator end() const {
        return others + N-1;
    }
    
};

template<unsigned int N>
class NaryClauses {
public:
    std::vector<std::vector<Occurrence<N>>> lists;

    NaryClauses() : lists() {}

    void init(unsigned int nVars) {
        lists.resize(2*nVars);
    }

    void clear() {
        lists.clear();
    }

    template<typename Iterator>
    void reinit(Iterator begin, Iterator end) {
        for (std::vector<Occurrence>& occ : lists) occ.clear();
        for (Iterator it = begin; it != end; it++) {
            Clause* clause = *it;
            if (clause->size() == N) {
                this->add(clause);
            }
        }
    }

    inline const std::vector<Occurrence>& operator [](Lit p) const {
        return lists[p];
    }

    void add(Clause* clause) {
        for (Lit lit : *clause) {
            lists[~lit].emplace_back(clause, lit);
        }
    }

    void remove(Clause* clause) {
        for (Lit lit : *clause) {
            lists[~lit].erase(std::find(lists[~lit].begin(), lists[~lit].end(), [clause](Occurrence o) { return o.clause == clause; }));
        }
    }

};

}

#endif