/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#ifndef MINIMIZE_H_
#define MINIMIZE_H_

#include <vector>
#include <map>

#include "candy/core/Solver.h"

namespace Candy {

class Minimizer {

private:
  CNFProblem& problem;
  Cl model;

  CNFProblem hittingSetProblem;

  /**
   * creates a sat-instance that models the set-cover-problem
   * that solves minimization of the given assignment;
   * pure literals that are set to false in the original problem
   * are stripped out before creating the new instance;
   * Iteratively minimize only maxIterations times (0 => until unsat)
   */
  Cl iterativeMinimization(CandySolverInterface* solver, Cl model);

public:
  Minimizer(CNFProblem& _problem, Cl _model);
  virtual ~Minimizer();

  void generateHittingSetProblem(For& clauses);
  CNFProblem& getHittingSetProblem();

  Cl computeMinimalModel(bool pruningActivated);

};

}

#endif /* MINIMIZE_H_ */
