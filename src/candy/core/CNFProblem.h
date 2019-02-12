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
    unsigned int maxVars = 0;
    bool emptyClause = false;

public:
    CNFProblem() { }

    CNFProblem(For& formula) {
        readClauses(formula);
    }

    CNFProblem(Cl& clause) {
        readClause(clause.begin(), clause.end());
    }

    CNFProblem(std::initializer_list<std::initializer_list<Lit>> formula) {
        readClauses(formula);
    }

    ~CNFProblem() {
        for (Cl* clause : problem) {
            delete clause;
        }
    }

    inline const For& getProblem() const {
        return problem;
    }

    inline For& getProblem() { 
        return problem;
    }

    void printDIMACS() const;

    bool hasEmptyClause() const {
        return emptyClause;
    }

    inline size_t nVars() const {
        return maxVars;
    }

    inline size_t nClauses() const {
        return problem.size();
    }

    inline int newVar() {
        maxVars++;
        return maxVars-1;
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
