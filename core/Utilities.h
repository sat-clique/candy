/*
 * Utilities.h
 *
 *  Created on: Aug 10, 2016
 *      Author: markus
 */

#ifndef CORE_UTILITIES_H_
#define CORE_UTILITIES_H_

#include "core/SolverTypes.h"

using namespace Glucose;

void printLiteral(Lit lit);
void printClause(Cl& clause);
void printClauses(For& formula);

#endif /* CORE_UTILITIES_H_ */
