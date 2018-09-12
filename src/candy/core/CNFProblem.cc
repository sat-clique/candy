/*
 * CNFProblem.cc
 *
 *  Created on: Mar 22, 2017
 *      Author: markus
 */

#include "candy/core/CNFProblem.h"

#include <vector>

namespace Candy {

std::vector<double> CNFProblem::getLiteralRelativeOccurrences() const {
    std::vector<double> literalOccurrence(maxVars*2, 0.0);
    for (auto c : problem) {
        for (Lit lit : *c) {
            literalOccurrence[lit] += 1.0 / c->size();
        }
    }
    double max = *std::max_element(literalOccurrence.begin(), literalOccurrence.end());
    for (double& occ : literalOccurrence) {
        occ = occ / max;
    }
    return literalOccurrence;
}

For& CNFProblem::getProblem() {
    return problem;
}

const For& CNFProblem::getProblem() const {
    return problem;
}

bool CNFProblem::hasEmptyClause() {
    for (Cl* clause : problem) {
        if (clause->size() == 0) return true;
    }
    return false;
}

bool CNFProblem::readDimacsFromStdin() {
    gzFile in = gzdopen(0, "rb");
    if (in == NULL) {
        printf("ERROR! Could not open file: <stdin>");
        return false;
    }
    readDimacs(in);
    gzclose(in);
    return true;
}

bool CNFProblem::readDimacsFromFile(const char* filename) {
    gzFile in = gzopen(filename, "rb");
    if (in == NULL) {
        printf("ERROR! Could not open file: %s\n", filename);
        return false;
    }
    readDimacs(in);
    gzclose(in);
    return true;
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
                    printf("PARSE ERROR! Expected positive occurence count in header but got %i vars and %i clauses\n", headerVars, headerClauses), exit(3); 
                }
                problem.reserve(headerClauses);
            }
            else {
                printf("PARSE ERROR! Expected 'p cnf' but got unexpected char: %c\n", *in), exit(3);
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
