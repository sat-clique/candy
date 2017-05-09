/*
 * Utilities.h
 *
 *  Created on: Aug 10, 2016
 *      Author: markus
 */

#ifndef UTILITIES_H
#define UTILITIES_H

#include "candy/core/SolverTypes.h"
#include "candy/core/Clause.h"
#include <vector>

void printClause(Candy::Cl& clause);
void printClauses(Candy::For& formula);

void printProblem(std::vector<Candy::Clause*> clauses, std::vector<Candy::lbool> values);
void printProblem(std::vector<Candy::Clause*> clauses);
void printClause(Candy::Clause& c, std::vector<Candy::lbool> values);
void printClause(Candy::Clause& c);

void printLiteral(Candy::Lit lit, std::vector<Candy::lbool> values);
void printLiteral(Candy::Lit lit);

#endif /* UTILITIES_H */
