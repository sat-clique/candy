/*
 * Utilities.cc
 *
 *  Created on: Aug 10, 2016
 *      Author: markus
 */

#include "core/Utilities.h"

void printLiteral(Lit lit) {
  printf("%s%i", sign(lit)?"-":"", var(lit)+1);
}

void printClause(Cl& clause) {
  for (Lit& lit : clause) {
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
