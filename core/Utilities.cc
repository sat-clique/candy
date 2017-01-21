/*
 * Utilities.cc
 *
 *  Created on: Aug 10, 2016
 *      Author: markus
 */

#include "core/Utilities.h"

void printLiteral(Candy::Lit lit) {
  printf("%s%i", sign(lit)?"-":"", var(lit)+1);
}

void printClause(Candy::Cl& clause) {
  for (Candy::Lit& lit : clause) {
    printLiteral(lit);
    printf(" ");
  }
  printf("0\n");
}

void printClauses(Candy::For& formula) {
  for (Candy::Cl* clause : formula) {
    printClause(*clause);
  }
}
