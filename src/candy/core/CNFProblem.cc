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

#include "candy/core/CNFProblem.h"
#include "candy/core/SolverTypes.h"
#include "candy/frontend/Exceptions.h"

#include <vector>

namespace Candy {

void CNFProblem::printDIMACS() const {
    printf("p cnf %zu %zu\n", nVars(), nClauses()); 
    for (Cl* clause : problem) {
        std::cout << *clause << "0" << std::endl;
    }
}

void CNFProblem::readDimacsFromStdin() {
    gzFile in = gzdopen(0, "rb");
    if (in == NULL) {
        throw ParserException("ERROR! Could not open file: <stdin>");
    }
    readDimacs(in);
    gzclose(in);
}

void CNFProblem::readDimacsFromFile(const char* filename) {
    gzFile in = gzopen(filename, "rb");
    if (in == NULL) {
        throw ParserException(std::string("ERROR! Could not open file") + filename);
    }
    readDimacs(in);
    gzclose(in);
}

void CNFProblem::readDimacs(gzFile input_stream) {
    unsigned int headerVars = 0;
    unsigned int headerClauses = 0;
    StreamBuffer in(input_stream);
    while (!in.eof()) {
        in.skipWhitespace();
        if (in.eof()) {
            break;
        }
        if (*in == 'p') {
            if (in.skipString("p cnf")) {
                int num1 = in.readInteger();
                int num2 = in.readInteger();
                if (num1 < 0 || num2 < 0) {
                    throw ParserException("PARSE ERROR! Expected positive occurence count in header but got " + std::to_string(headerVars) + " vars and " + std::to_string(headerClauses) + " clauses");
                }
                else {
                    headerVars = num1;
                    headerClauses = num2;
                }
                problem.reserve(headerClauses);
            }
            else {
                throw ParserException("PARSE ERROR! Expected 'p cnf' but got unexpected char: " + std::to_string(*in));
            }
        }
        else if (*in == 'c') {
            in.skipLine();
        }
        else {
            static Cl lits;
            lits.clear();
            for (int plit = in.readInteger(); plit != 0; plit = in.readInteger()) {
                lits.push_back(Lit(abs(plit)-1, plit < 0));
                if (in.eof()) {
                    throw ParserException("PARSE ERROR! Expected clause but got unexpected char: " + std::to_string(*in));
                }
            }
            readClause(lits.begin(), lits.end());
        }
    }
    if (headerVars != maxVars) {
        fprintf(stderr, "c WARNING! DIMACS header mismatch: wrong number of variables (declared %i, found %i).\n", headerVars, maxVars);
    }
    if (headerClauses != problem.size()) {
        fprintf(stderr, "c WARNING! DIMACS header mismatch: wrong number of clauses (declared %i, found %i).\n", headerClauses, (int)problem.size());
    }
}

void CNFProblem::readClause(std::initializer_list<Lit> list) {
    readClause(list.begin(), list.end());
}

void CNFProblem::readClause(Cl& cl) {
    readClause(cl.begin(), cl.end());
}

void CNFProblem::readClauses(std::initializer_list<std::initializer_list<Lit>> f) {
    for (std::initializer_list<Lit> c : f) {
        readClause(c);
    }
}

void CNFProblem::readClauses(For& f) {
    for (Cl* c : f) {
        readClause(c->begin(), c->end());
    }
}

template <typename Iterator>
void CNFProblem::readClause(Iterator begin, Iterator end) {
    if (std::distance(begin, end) > 0) {
        Cl* clause = new Cl(begin, end);
        std::sort(clause->begin(), clause->end());
        // remove redundant literals
        clause->erase(std::unique(clause->begin(), clause->end()), clause->end());
        // skip tatological clause
        bool isTautological = clause->end() != std::unique(clause->begin(), clause->end(), [](Lit l1, Lit l2) { return l1.var() == l2.var(); });
        if (isTautological) {
            delete clause;
            return;
        }
        maxVars = std::max(maxVars, (unsigned int) 1 + clause->back().var()); 
        problem.push_back(clause);
    }
    else {
        problem.push_back(new Cl());
    }
}

}
