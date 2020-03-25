#include <zlib.h>

#include <string>
#include <fstream>
#include <streambuf>

#include <gtest/gtest.h>

#include "candy/core/CNFProblem.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/frontend/CandyBuilder.h"

using namespace Candy;

TEST (CandyAddClauseTestPatterns, addClause) {
  CNFProblem formula({{1_L, 2_L}});
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->nClauses(), 1ul);
}

TEST (CandyAddClauseTestPatterns, rejectTautologies) {
  CNFProblem formula({{1_L, ~1_L}});
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->nClauses(), 0ul);
}

TEST (CandyAddClauseTestPatterns, rejectTautologies2) {
  CNFProblem formula({{1_L, 2_L, ~1_L}});
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->nClauses(), 0ul);
}

TEST (CandyAddClauseTestPatterns, materialUnitClauses) {
  CNFProblem formula({{1_L}});
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->nClauses(), 1ul);
}

TEST (CandyAddClauseTestPatterns, materialUnitClausesAndNoEagerUnitPropagation) {
  CNFProblem formula({{1_L}, {1_L, 2_L}});
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->nClauses(), 2ul);
}

TEST (CandyAddClauseTestPatterns, materialUnitClausesAndNoEagerUnitPropagation2) {
  CNFProblem formula({{~1_L}, {1_L, 2_L}});
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->nClauses(), 2ul);
}

TEST(SolverTests, basicIncrementalSolver) {
  CNFProblem problem { {{1_L, 2_L}, {1_L, 1_L}, {2_L, 2_L}} };
  CandySolverInterface* solver = createSolver();
  Var assumptionVar = problem.newVar();
  solver->init(problem);
  solver->getAssignment().setAssumptions(Cl { Lit(assumptionVar, 0) });
  EXPECT_TRUE(l_True == solver->solve());
  solver->getAssignment().setAssumptions(Cl { Lit(assumptionVar, 1) });
  EXPECT_TRUE(l_True == solver->solve());
}
