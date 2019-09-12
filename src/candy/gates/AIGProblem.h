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

#ifndef AIG_PROBLEM
#define AIG_PROBLEM

#include <cstdlib>
#include <algorithm>
#include <vector>
#include <set>

#include "candy/core/SolverTypes.h"
#include "candy/gates/GateProblem.h"

namespace Candy {

class AndGate {
public:
    AndGate(Lit output_, Cl conj_)
	 : output(output_), conj(conj_) {}
	
	Lit output;
    Cl conj;
};

class BinaryAndGate {
public:
    BinaryAndGate(Lit output_, Lit left_, Lit right_)
	 : output(output_), left(left_), right(right_) {}

	Lit output;
	Lit left;
	Lit right;
};

class AIGProblem {
    const GateProblem& problem;

    unsigned int var_count;

    std::vector<Lit> literals;
    std::set<Var> inputs;
    std::vector<bool> visited;

    std::vector<AndGate> pands;
    std::vector<BinaryAndGate> ands;

public:
    AIGProblem(const GateProblem& p)
     : problem(p), var_count(p.nVars()), literals(), inputs(), visited(), pands() {
        if (problem.hasArtificialRoot()) {
            var_count++;
        }
        visited.resize(2 * var_count, false);
        literals.push_back(problem.getRoot());
    }

    ~AIGProblem() {
    }

    unsigned int nVars() const {
        return var_count;
    }

    void createAIG() {
        createAnds();
        convertNaryAndsToBinaryAnds();
    }

    void printLit(FILE* out, Lit lit, std::vector<bool>& negate) {
        if (lit == lit_Undef) {
            fprintf(out, "1");
        } else {
            fprintf(out, "%i", (int)(negate[lit.var()] ? ~lit : lit));
        }
    }

    void printAIG(FILE* out) {
        // As output may not be negative in AIG format we need to flip these globally:
        std::vector<bool> negate(var_count, false);
        for (BinaryAndGate a : ands) {
            if (a.output.sign()) negate[a.output.var()] = true;
        }

        fprintf(out, "aag %i %zu %i %i %zu\n", var_count, inputs.size(), 0, 1, ands.size());
        for (Var var : inputs) {
            printLit(out, Lit(var), negate);
            fprintf(out, "\n");
        }
        printLit(out, problem.getRoot(), negate);
        fprintf(out, "\n");

        for (BinaryAndGate a : ands) {
            printLit(out, a.output, negate);
            fprintf(out, " ");
            printLit(out, a.left, negate);
            fprintf(out, " ");
            printLit(out, a.right, negate);
            fprintf(out, "\n");
        }
    }

private:
    void registerLiteral(Lit lit) {
        if (problem[lit.var()].isDefined()) {
            literals.push_back(lit);
        } else {
            inputs.insert(lit.var());
        }
    }

    void createAndFromClauses(For& disjunctions, Lit output) {
        Cl conj;
        for (Cl* clause : disjunctions) {
            if (clause->size() == 1) {
                registerLiteral(clause->front());
                conj.push_back(clause->front());
            } 
            else {
                Lit out = Lit(++var_count);
                conj.push_back(out);

                Cl inverted;
                for (Lit lit : *clause) {
                    registerLiteral(lit);
                    inverted.push_back(~lit);
                }
                pands.emplace_back(~out, inverted);
            }
        }
        pands.emplace_back(output, conj);
    }

    void createAnds() {
        while (literals.size() > 0) {
            Lit lit = literals.back();
            literals.pop_back();

            if (!visited[lit]) {
                visited[lit] = true;

                if (problem[lit.var()].isDefined()) { 
                    For& clauses = (For&)problem[lit.var()].fwd;

                    // strip output-literal from clauses
                    For list;
                    for (Cl* clause : clauses) {
                        Cl* cl = new Cl(clause->size()-1);
                        remove_copy(clause->begin(), clause->end(), cl->begin(), ~lit);
                        list.push_back(cl);
                    }

                    createAndFromClauses(list, lit);

                    for (Cl* c : list) delete c;
                }
            }
        }
    }

    Lit convertNaryRecursive(Cl& conj) {
        if (conj.size() == 1) {
            return conj.front();
        }
        else {
            int pivot = conj.size() / 2;
            Cl lefts { conj.begin(), conj.begin() + pivot };
            Cl rights { conj.begin() + pivot, conj.end() };
            Lit left = convertNaryRecursive(lefts);
            Lit right = convertNaryRecursive(rights);
            Lit output = Lit(++var_count);
            ands.emplace_back(output, left, right);
            return output;
        }
    }

    void convertNaryAndsToBinaryAnds() {
        for (AndGate a : pands) {
            if (a.conj.size() == 0) {
                fprintf(stderr, "Warning: Conjunction Size is Zero at %i\n ", (int)a.output);
            }
            else if (a.conj.size() == 1) {
                ands.emplace_back(a.output, a.conj.front(), lit_Undef);
            }
            else {
                int pivot = a.conj.size() / 2;
                Cl lefts { a.conj.begin(), a.conj.begin() + pivot };
                Cl rights { a.conj.begin() + pivot, a.conj.end() };
                Lit left = convertNaryRecursive(lefts);
                Lit right = convertNaryRecursive(rights);
                ands.emplace_back(a.output, left, right);
            }
        }
    }

};

}

#endif