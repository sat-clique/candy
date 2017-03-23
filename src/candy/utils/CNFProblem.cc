/*
 * CNFProblem.cc
 *
 *  Created on: Mar 22, 2017
 *      Author: markus
 */

#include "candy/utils/CNFProblem.h"

#include "utils/ParseUtils.h"
#include "core/SolverTypes.h"
#include <vector>

namespace Candy {

CNFProblem::CNFProblem() { }

For& CNFProblem::getProblem() {
    return problem;
}

const For& CNFProblem::getProblem() const {
    return problem;
}

int CNFProblem::nVars() const {
    return maxVars;
}

int CNFProblem::nClauses() const {
    return (int)problem.size();
}

int CNFProblem::newVar() {
    maxVars++;
    return maxVars-1;
}

bool CNFProblem::readDimacsFromStdout() {
    gzFile in = gzdopen(0, "rb");
    if (in == NULL) {
        printf("ERROR! Could not open file: <stdin>");
        return false;
    }
    parse_DIMACS(in);
    gzclose(in);
    return true;
}

bool CNFProblem::readDimacsFromFile(const char* filename) {
    gzFile in = gzopen(filename, "rb");
    if (in == NULL) {
        printf("ERROR! Could not open file: %s\n", filename);
        return false;
    }
    parse_DIMACS(in);
    gzclose(in);
    return true;
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

template <typename Iterator>
void CNFProblem::readClause(Iterator begin, Iterator end) {
    maxVars = std::max(maxVars, var(*std::max_element(begin, end))+1);
    problem.push_back(new Cl(begin, end));
}

void CNFProblem::readClauses(For& f) {
    for (Cl* c : f) {
        readClause(c->begin(), c->end());
    }
}

void CNFProblem::readClause(Glucose::StreamBuffer& in) {
    static Cl lits;
    lits.clear();
    for (int plit = parseInt(in); plit != 0; plit = parseInt(in)) {
        lits.push_back(mkLit(abs(plit)-1, plit < 0));
    }
    readClause(lits.begin(), lits.end());
}

void CNFProblem::parse_DIMACS(gzFile input_stream) {
    Glucose::StreamBuffer in(input_stream);
    skipWhitespace(in);
    while (*in != EOF) {
        if (*in == 'p') {
            if (eagerMatch(in, "p cnf")) {
                headerVars = parseInt(in);
                headerClauses = parseInt(in);
                problem.reserve(headerClauses);
            }
            else {
                printf("PARSE ERROR! Unexpected char: %c\n", *in), exit(3);
            }
        }
        else if (*in == 'c' || *in == 'p') {
            skipLine(in);
        }
        else {
            readClause(in);
        }
        skipWhitespace(in);
    }
    if (headerVars != maxVars) {
        fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of variables.\n");
    }
    if (headerClauses != (int)problem.size()) {
        fprintf(stderr, "WARNING! DIMACS header mismatch: wrong number of clauses.\n");
    }
}

}
