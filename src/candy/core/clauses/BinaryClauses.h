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

#ifndef SRC_CANDY_CORE_BINARIES_H_
#define SRC_CANDY_CORE_BINARIES_H_

#include "candy/core/SolverTypes.h"

#include "candy/core/clauses/Clause.h"

namespace Candy {

class BinaryClauses {
public:
    std::vector<std::vector<Lit>> binary_watchers;

    BinaryClauses() : binary_watchers() {}

    void init(unsigned int nVars) {
        binary_watchers.resize(2*nVars);
    }

    void clear() {
        binary_watchers.clear();
    }

    template<typename Iterator>
    void reinit(Iterator begin, Iterator end) {
        for (std::vector<Lit>& w : binary_watchers) w.clear();
        for (Iterator it = begin; it != end; it++) {
            Clause* clause = *it;
            if (clause->size() == 2) {
                this->add(clause);
            }
        }
    }

    inline const std::vector<Lit>& operator [](int i) const {
        return binary_watchers[i];
    }

    void add(Clause* clause) {
        binary_watchers[~clause->first()].push_back(clause->second());
        binary_watchers[~clause->second()].push_back(clause->first());
    }

    void remove(Clause* clause) {
        std::vector<Lit>& list0 = binary_watchers[~clause->first()];
        std::vector<Lit>& list1 = binary_watchers[~clause->second()];
        list0.erase(std::remove(list0.begin(), list0.end(), clause->second()), list0.end());
        list1.erase(std::remove(list1.begin(), list1.end(), clause->first()), list1.end());
    }

};

}

#endif