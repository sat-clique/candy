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

#ifndef SRC_CANDY_CORE_EQIV_H_
#define SRC_CANDY_CORE_EQIV_H_

#include "candy/core/SolverTypes.h"

#include "candy/core/clauses/BinaryClauses.h"

#include "candy/mtl/Stamp.h"
#include "candy/mtl/State.h"

#include <vector>
#include <stack>

namespace Candy {

class Equivalences {
    BinaryClauses& binary_clauses;

    std::vector<Lit> equiv;

    Stamp<uint8_t> block;
	State<uint8_t, 3> mark;
	std::vector<Lit> parent;
	std::stack<Lit> stack;

public:
    Equivalences(BinaryClauses& binary_clauses_) : 
        binary_clauses(binary_clauses_), equiv(), 
        block(), mark(), parent(), stack() {}

    void init(unsigned int nVars) {
        unsigned int i = equiv.size();
        equiv.resize(nVars);
        for (; i < nVars; i++) {
            equiv[i] = Lit(i);
        }
        block.grow(2*nVars);
        mark.grow(2*nVars);
        parent.resize(2*nVars, lit_Undef);
    }

    void clear() {
        equiv.clear();
    }

	void greedy_cycle(Lit root) {
        // std::stringbuf buf;
        // std::ostream os(&buf);
        if (block[root]) return;
        block.set(root);
        mark.clear();
        std::fill(parent.begin(), parent.end(), lit_Undef);
		stack.push(root);
		mark.set(root, 2);
		while (!stack.empty()) {
			Lit lit = stack.top(); stack.pop();
			for (const BinaryWatcher& watcher : binary_clauses[lit]) {
                Lit impl = watcher.other;
				if (mark[impl] == 0) {
					parent[impl] = lit;
					mark.set(impl, 1);
					stack.push(impl);
                    // os << *child.clause << std::endl;
				}
				else if (mark[impl] == 2) {
                    // std::cout << buf.str();
                    // std::cout << "found cycle on root " << root << " with clause " << *child.clause << ": ";
                    // for (Lit iter = parent[lit.var()]; iter != lit_Undef && mark[iter] != 2; iter = parent[iter.var()]) std::cout << " " << iter << " <-> " << lit;
                    // std::cout << std::endl;
					parent[impl] = lit;
                    Lit iter = impl;
                    while (mark[parent[iter]] != 2) {
                        block.set(iter);
                        set_eq(parent[iter], impl);
						mark.set(iter, 2);
                        iter = parent[iter];
                    }
				}
			}
		}
	}

    void set_eq(Lit lit1, Lit lit2) {
        Lit rep1 = get_eq(lit1);
        Lit rep2 = get_eq(lit2);
        equiv[rep1.var()] = rep1.sign() ? ~rep2 : rep2;
    }

    Lit get_eq(Lit lit) {
        return lit.sign() ? ~get_eq(lit.var()) : get_eq(lit.var());
    }

    Lit get_eq(Var v) {
        Lit rep = equiv[v];
        if (rep.var() != v) rep = rep.sign() ? ~get_eq(rep.var()) : get_eq(rep.var());
        equiv[v] = rep;
        return rep;
    }

};

}

#endif