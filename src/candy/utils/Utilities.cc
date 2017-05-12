/*
 * Utilities.cc
 *
 *  Created on: Aug 10, 2016
 *      Author: markus
 */

#include "candy/utils/Utilities.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/Clause.h"

using namespace Candy;

void printLiteral(Lit lit) {
  printf("%s%i", sign(lit)?"-":"", var(lit)+1);
}

void printLiteral(Lit lit, std::vector<lbool> values) {
    lbool value = values[var(lit)] ^ sign(lit);
    printf("%s%d:%c", sign(lit) ? "-" : "", var(lit) + 1, value == l_True ? '1' : (value == l_False ? '0' : 'X'));
}

void printClause(Cl& clause) {
  for (Lit& lit : clause) {
    printLiteral(lit);
    printf(" ");
  }
  printf("0\n");
}

void printClause(Clause& c, std::vector<lbool> values) {
    for (Lit lit : c) {
        printLiteral(lit, values);
        printf(" ");
    }
    printf("0\n");
}

void printClause(Clause& c) {
    for (Lit lit : c) {
        printLiteral(lit);
        printf(" ");
    }
    printf("0\n");
}

void printClauses(For& formula) {
  for (Cl* clause : formula) {
    printClause(*clause);
  }
}

void printProblem(std::vector<Clause*> clauses, std::vector<lbool> values) {
    for (auto clause : clauses) {
        printClause(*clause, values);
        printf("\n");
    }
}

void printProblem(std::vector<Clause*> clauses) {
    int maxVar = 0;
    for (auto clause : clauses) {
        for (Lit lit : *clause) {
            if (var(lit) > maxVar) {
                maxVar = var(lit);
            }
        }
    }
    printf("p cnf %i %i\n", maxVar, clauses.size());
    for (auto clause : clauses) {
        printClause(*clause);
    }
}
