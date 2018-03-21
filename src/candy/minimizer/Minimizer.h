/*
 * Minimize.h
 *
 *  Created on: 10.01.2013
 *      Author: markus
 */

#ifndef MINIMIZE_H_
#define MINIMIZE_H_

#include <vector>
#include <map>

#include "candy/simp/SimpSolver.h"

namespace Candy {

class Minimizer {

private:
  CNFProblem* clauses;

  /**
   * creates a sat-instance that models the set-cover-problem
   * that solves minimization of the given assignment;
   * pure literals that are set to false in the original problem
   * are stripped out before creating the new instance;
   * Iteratively minimize only maxIterations times (0 => until unsat)
   */
  Cl iterativeMinimization(CandySolverInterface* solver, Cl model);

public:
  Minimizer(CNFProblem* problem);
  virtual ~Minimizer();

  CNFProblem* generateHittingSetProblem(For clauses, Cl model);

  Cl computeMinimalModel(Cl model, bool pruningActivated);

};

}

#endif /* MINIMIZE_H_ */
