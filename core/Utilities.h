/*
 * Utilities.h
 *
 *  Created on: Aug 10, 2016
 *      Author: markus
 */

#ifndef CORE_UTILITIES_H_
#define CORE_UTILITIES_H_

#include "core/SolverTypes.h"

void printLiteral(Candy::Lit lit);
void printClause(Candy::Cl& clause);
void printClauses(Candy::For& formula);

#endif /* CORE_UTILITIES_H_ */
