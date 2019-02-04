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

#ifndef CNFProblem_h
#define CNFProblem_h

#include "candy/utils/StreamBuffer.h"
#include "candy/core/SolverTypes.h"

#include <vector>

namespace Candy {

class CNFProblem {

private:
    For problem;

    int maxVars = 0;

    int headerVars = 0;
    int headerClauses = 0;

public:
    CNFProblem() {
    }

    ~CNFProblem() {
        for (Cl* clause : problem) {
            delete clause;
        }
    }

    For& getProblem();
    const For& getProblem() const;
    void printDIMACS() const;

    bool hasEmptyClause();

    inline int nVars() const {
        return maxVars;
    }

    inline int nClauses() const {
        return (int)problem.size();
    }

    inline int newVar() {
        maxVars++;
        return maxVars-1;
    }

    bool isSatisfied(Cl model) {
        sort(model.begin(), model.end(), [](Lit lit1, Lit lit2) { return var(lit1) < var(lit2); });
        for (Cl* clause : problem) {
            bool sat = false;
            for (Lit lit : *clause) {
                assert(var(model[var(lit)]) == var(lit));
                sat |= model[var(lit)] == lit;
            }
            if (!sat) {
                std::cout << "c Error! Clause is not satisfied by model: " << *clause << std::endl;
                return false;
            }
        }
        return true;
    }

    std::vector<double> getLiteralRelativeOccurrences() const;

    void readDimacsFromStdin();
    void readDimacsFromFile(const char* filename);

    void readClause(Lit plit);
    void readClause(Lit plit1, Lit plit2);
    void readClause(std::initializer_list<Lit> list);
    void readClause(Cl cl);
    void readClauses(std::initializer_list<std::initializer_list<Lit>> f);
    void readClauses(For& f);

    template <typename Iterator>
    void readClause(Iterator begin, Iterator end);

    // CNFProblem can only be moved, not copied
    CNFProblem(const CNFProblem& other) = delete;
    CNFProblem& operator=(const CNFProblem& other) = delete;
    CNFProblem& operator=(CNFProblem&& other) = default;
    CNFProblem(CNFProblem&& other) = default;

private:

    void readDimacs(gzFile input_stream);

};

}

#endif
