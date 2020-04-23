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

#include "candy/utils/StreamBuffer.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/CandySolverResult.h"
#include "candy/core/SolverTypes.h"
#include "candy/utils/Exceptions.h"

#include <unordered_map>

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <errno.h>
#include <string.h>

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
        throw ParserException(std::string("ERROR! Could not open file"));
    }
    readDimacs(in);
    gzclose(in);
}

void CNFProblem::readDimacs(gzFile input_stream) {
    Cl lits;
    StreamBuffer in(input_stream);
    while (!in.eof()) {
        in.skipWhitespace();
        if (in.eof()) {
            break;
        }
        if (*in == 'p') {
            in.skipString("p cnf");
            int headerVars = in.readInteger();
            int headerClauses = in.readInteger();
            if (headerVars < 0 || headerClauses < 0) {
                throw ParserException("PARSE ERROR! Expected positive occurence count in header but got " + std::to_string(headerVars) + " vars and " + std::to_string(headerClauses) + " clauses");
            }
            problem.reserve(headerClauses);
        }
        else if (*in == 'c') {
            in.skipLine();
        }
        else {
            lits.clear();
            for (int plit = in.readInteger(); plit != 0; plit = in.readInteger()) {
                lits.push_back(Lit(abs(plit)-1, plit < 0));
            }
            readClause(lits.begin(), lits.end());
        }
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
    Cl* clause = new Cl(begin, end);
    if (clause->size() > 0) {
        std::sort(clause->begin(), clause->end());
        // record maximal variable
        variables = std::max(variables, (unsigned int)clause->back().var()+1); 
        // remove redundant literals
        clause->erase(std::unique(clause->begin(), clause->end()), clause->end());
        // skip tatological clause
        bool isTautological = clause->end() != std::unique(clause->begin(), clause->end(), [](Lit l1, Lit l2) { return l1.var() == l2.var(); });
        if (isTautological) {
            delete clause;
            return;
        }
    }
    problem.push_back(clause);
}

bool CNFProblem::checkResult(CandySolverResult& result) {
    for (Cl* clause : problem) {
        bool satisfied = false;
        for (Lit lit : *clause) {
            if (result.satisfies(lit)) {
                satisfied = true; 
                break;
            }
        }
        if (!satisfied) {
            std::cout << "c Clause not satisfied: " << *clause << std::endl;
            std::cout << "c Values: ";
            for (Lit lit : *clause) {
                std::cout << "c (" << lit.var() << ", " << result.value(lit.var()) << ")" << std::endl;
            }
            return false;
        }
    }
    return true;
}

/**
 * Use with care: renames all variables for as gap-less representation.
 * Don't use or translate back, if you care for semantics.
 * */
void CNFProblem::normalizeVariableNames() {
    std::unordered_map<Var, Var> name;
    //name.resize(variables, -1);
    unsigned int max = 0;
    for (Cl* clause : problem) {
        for (Lit& lit : *clause) {
            if (name[lit.var()] == -1) name[lit.var()] = max++;
            lit = Lit(name[lit.var()], lit.sign());
        }
    }
    variables = max;
}

}
