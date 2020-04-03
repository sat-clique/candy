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

#ifndef CANDY_GATES_BLOCKLIST_H_
#define CANDY_GATES_BLOCKLIST_H_

#include <vector>
#include <set>
#include <limits>

#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"

namespace Candy {

class BlockList {

private:
    const CNFProblem& problem;

    std::vector<For> index;
    std::vector<uint16_t> num_blocked;

    const uint16_t num_blocked_invalid = std::numeric_limits<uint16_t>::max();

    bool isBlocked(Lit o, Cl& c1, Cl& c2) const { // assert o \in c1 and ~o \in c2
        for (Lit l1 : c1) if (l1 != o) for (Lit l2 : c2) if (l1 == ~l2) return true;
        return false;
    }

    bool isBlocked(Lit o, Cl* clause) const { // assert o \in clause
        for (Cl* c2 : index[~o]) if (!isBlocked(o, *clause, *c2)) return false;
        return true;
    }

    void countBlocked(Lit o) {
        num_blocked[o] = 0;
        for (Cl* clause : index[o]) {
            if (isBlocked(o, clause)) {
                num_blocked[o]++;
            }
        }
    }

public:
    BlockList(const CNFProblem& problem_) : problem(problem_) { 
        index.resize(2 * problem.nVars());
        num_blocked.resize(2 * problem.nVars(), num_blocked_invalid);

        for (Cl* clause : problem_) {
            for (Lit lit : *clause) {
                index[lit].push_back(clause);
            }
        }
    }

    ~BlockList() { }

    uint16_t getNumBlocked(Lit o) {
        if (num_blocked[o] == num_blocked_invalid) {
            countBlocked(o);
        }
        return num_blocked[o];
    }

    void remove(For& clauses) {
        for (Cl* clause : clauses) {
            for (Lit lit : *clause) {
                For& h = index[lit];
                h.erase(std::remove(h.begin(), h.end(), clause), h.end());
                num_blocked[lit] = num_blocked_invalid;
                num_blocked[~lit] = num_blocked_invalid;
            }
        }
    }

    inline const For& operator [](size_t o) const {
        return index[o];
    }

    inline size_t size() const {
        return index.size();
    }

    inline bool isBlockedSet(Lit o) {
        return index[o].size() == getNumBlocked(o);
    }

    Lit getMinimallyUnblockedLiteral() {
        Lit result = lit_Undef;
        uint16_t min = std::numeric_limits<uint16_t>::max();
        for (int v = problem.nVars()-1; v >= 0 && min > 1; v--) {
            for (Lit lit : { Lit(v, true), Lit(v, false) }) {
                size_t total = index[lit].size();
                size_t diff = total - getNumBlocked(lit);
                if (diff > 0 && diff < min) {
                    min = (uint16_t)diff;
                    result = lit;
                }
            }
        }
        return result;
    }

    For stripUnblockedClauses(Lit o) {
        For result;
        
        for (Cl* clause : index[o]) {
            if (!isBlocked(o, clause)) {
                result.push_back(clause);
            }
        }

        for (Cl* clause : result) {
            for (Lit lit : *clause) {
                For& h = index[lit];
                h.erase(std::remove(h.begin(), h.end(), clause), h.end());
                if (lit != o) {
                    num_blocked[lit] = num_blocked_invalid;
                    num_blocked[~lit] = num_blocked_invalid;
                }
                else {
                    num_blocked[~lit] = index[~lit].size();
                }
            }
        }

        return result;
    }

};

}
#endif