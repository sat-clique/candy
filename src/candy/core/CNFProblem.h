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

#include "candy/core/SolverTypes.h"

typedef struct gzFile_s *gzFile;

namespace Candy {

class CandySolverResult;

class CNFProblem {

private:
    For problem;
    unsigned int variables;

public:
    CNFProblem() : variables(0) { }

    CNFProblem(For& formula) : variables(0) {
        readClauses(formula);
    }

    CNFProblem(Cl& clause) : variables(0) {
        readClause(clause.begin(), clause.end());
    }

    CNFProblem(std::initializer_list<std::initializer_list<Lit>> formula) : variables(0) {
        readClauses(formula);
    }

    ~CNFProblem() {
        clear();
    }

    typedef For::const_iterator const_iterator;

    inline const_iterator begin() const {
        return problem.begin();
    }

    inline const_iterator end() const {
        return problem.end();
    }

    inline const Cl* operator [](int i) const {
        return problem[i];
    }

    inline size_t nVars() const {
        return variables;
    }

    inline size_t nClauses() const {
        return problem.size();
    }

    inline int newVar() {
        return variables++;
    }

    inline void clear() {
        for (Cl* clause : problem) {
            delete clause;
        }
        problem.clear();
    }

    void normalizeVariableNames();
    bool checkResult(CandySolverResult& result);

    void printDIMACS() const;

    void readDimacsFromStdin();
    void readDimacsFromFile(const char* filename);

    void readClause(std::initializer_list<Lit> list);
    void readClause(Cl& cl);
    
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
