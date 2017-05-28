/*
 * CNFProblem.cc
 *
 *  Created on: Mar 22, 2017
 *      Author: markus
 */

#include "candy/utils/CNFProblem.h"

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
                printf("PARSE ERROR! Expected 'p cnf' but got unexpected char: %c\n", *in), exit(3);
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
