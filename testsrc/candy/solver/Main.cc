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
  CNFProblem formula;
  formula.readClause({mkLit(1), mkLit(2)});
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->getStatistics().nClauses(), 1ul);
}

TEST (CandyAddClauseTestPatterns, rejectTautologies) {
  CNFProblem formula;
  formula.readClause(mkLit(1), mkLit(1, true));
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->getStatistics().nClauses(), 0ul);
}

TEST (CandyAddClauseTestPatterns, rejectTautologies2) {
  CNFProblem formula;
  formula.readClause({ mkLit(1), mkLit(2), mkLit(1, true) });
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->getStatistics().nClauses(), 0ul);
}

TEST (CandyAddClauseTestPatterns, materialUnitClauses) {
  CNFProblem formula;
  formula.readClause({mkLit(1)});
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->getStatistics().nClauses(), 1ul);
}

TEST (CandyAddClauseTestPatterns, materialUnitClauses2) {
  CNFProblem formula;
  // assuming claues are added to solver in the given order
  formula.readClause(mkLit(1));
  formula.readClause(mkLit(1), mkLit(2));  
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->getStatistics().nClauses(), 2ul); 
  // ASSERT_TRUE(clauses[0]->isDeleted());
}

TEST (CandyAddClauseTestPatterns, materialUnitClauses3) {
  CNFProblem formula;
  formula.readClause(mkLit(1, true));
  formula.readClause(mkLit(1), mkLit(2));
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->getStatistics().nClauses(), 3ul); 
  // ASSERT_TRUE(clauses[0]->isDeleted());
}

TEST (CandyAddClauseTestPatterns, lazyDeletionOfStrengthenedClauses) {
  CNFProblem formula;
  formula.readClause(mkLit(1, true));
  formula.readClause({mkLit(1), mkLit(2), mkLit(3)});
  CandySolverInterface* solver = createSolver();
  solver->init(formula); 
  ASSERT_EQ(solver->getStatistics().nClauses(), 3ul);
  // ASSERT_EQ(clauses[0]->size(), 3ul);
  // ASSERT_TRUE(clauses[0]->isDeleted());
  // ASSERT_EQ(clauses[1]->size(), 2ul);
  // ASSERT_FALSE(clauses[1]->isDeleted());
}

TEST(SolverTests, basicIncrementalSolver) {
  CNFProblem problem { {{1_L, 2_L}, {1_L, 1_L}, {2_L, 2_L}} };
  CandySolverInterface* solver = createSolver();
  Var assumptionVar = problem.newVar();
  solver->init(problem);
  solver->setAssumptions(Cl { mkLit(assumptionVar, 0) });
  EXPECT_TRUE(l_True == solver->solve());
  solver->setAssumptions(Cl { mkLit(assumptionVar, 1) });
  EXPECT_TRUE(l_True == solver->solve());
}
