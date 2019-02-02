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

std::vector<double> CNFProblem::getLiteralRelativeOccurrences() const {
    std::vector<double> literalOccurrence(maxVars*2, 0.0);

    if (nVars() > 0) {
        for (auto c : problem) {
            for (Lit lit : *c) {
                literalOccurrence[lit] += 1.0 / c->size();
            }
        }
        double max = *std::max_element(literalOccurrence.begin(), literalOccurrence.end());
        for (double& occ : literalOccurrence) {
            occ = occ / max;
        }
    }
    
    return literalOccurrence;
}

For& CNFProblem::getProblem() {
    return problem;
}

const For& CNFProblem::getProblem() const {
    return problem;
}

void CNFProblem::printDIMACS() const {
    printf("p cnf %zu %zu\n", nVars(), nClauses()); 
    for (Cl* clause : problem) {
        std::cout << *clause << "0" << std::endl;
    }
}

bool CNFProblem::hasEmptyClause() {
    for (Cl* clause : problem) {
        if (clause->size() == 0) return true;
    }
    return false;
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
    StreamBuffer in(input_stream);
    while (!in.eof()) {
        in.skipWhitespace();
        if (in.eof()) {
            break;
        }
        if (*in == 'p') {
            if (in.skipString("p cnf")) {
                headerVars = in.readInteger();
                headerClauses = in.readInteger();
                if (headerVars < 0 || headerClauses < 0) {
                    throw ParserException("PARSE ERROR! Expected positive occurence count in header but got " + std::to_string(headerVars) + " vars and " + std::to_string(headerClauses) + " clauses");
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
                lits.push_back(mkLit(abs(plit)-1, plit < 0));
                if (in.eof()) {
                    throw ParserException("PARSE ERROR! Expected clause but got unexpected char: " + std::to_string(*in));
                }
            }
            readClause(lits.begin(), lits.end());
        }
    }
    if (headerVars != maxVars) {
        fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of variables (declared %i, found %i).\n", headerVars, maxVars);
    }
    if (headerClauses != (int)problem.size()) {
        fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of clauses (declared %i, found %i).\n", headerClauses, (int)problem.size());
    }
}

void CNFProblem::readClause(Lit plit) {
    readClause({plit});
}

void CNFProblem::readClause(Lit plit1, Lit plit2) {
    readClause({plit1, plit2});
}

void CNFProblem::readClause(std::initializer_list<Lit> list) {
    readClause(list.begin(), list.end());
}

void CNFProblem::readClause(Cl cl) {
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
    Cl* clause = new Cl(begin, end);
    for (Lit lit : *clause) {
        maxVars = std::max(maxVars, 1 + var(lit));
    }
    problem.push_back(clause);
}

}
