#include <zlib.h>

#include <string>
#include <fstream>
#include <streambuf>

#include <gtest/gtest.h>

#include <candy/core/CNFProblem.h>
#include <candy/core/Solver.h>
#include <candy/frontend/CandyBuilder.h>

using namespace Candy;

TEST (CandyAddClauseTestPatterns, addClause) {
  CNFProblem formula;
  formula.readClause({mkLit(1), mkLit(2)});
  Solver<> solver;
  solver.addClauses(formula);
  ASSERT_EQ(solver.nClauses(), 1ul);
}

TEST (CandyAddClauseTestPatterns, rejectTautologies) {
  CNFProblem formula;
  formula.readClause(mkLit(1), mkLit(1, true));
  Solver<> solver;
  solver.addClauses(formula);
  ASSERT_EQ(solver.nClauses(), 0ul);
}

TEST (CandyAddClauseTestPatterns, rejectTautologies2) {
  CNFProblem formula;
  formula.readClause({ mkLit(1), mkLit(2), mkLit(1, true) });
  Solver<> solver;
  solver.addClauses(formula);
  ASSERT_EQ(solver.nClauses(), 0ul);
}

TEST (CandyAddClauseTestPatterns, propagateEarly) {
  CNFProblem formula;
  formula.readClause({mkLit(1)});
  Solver<> solver;
  solver.addClauses(formula);
  ASSERT_EQ(solver.nClauses(), 0ul);
}

TEST (CandyAddClauseTestPatterns, unitResolution) {
  CNFProblem formula;
  // assuming claues are added to solver in the given order
  formula.readClause(mkLit(1));
  formula.readClause(mkLit(1), mkLit(2));
  ClauseDatabase<>* clauses = new ClauseDatabase<>();
  CandyBuilder<> builder { clauses, new Trail() };
  CandySolverInterface* solver = builder.build();
  solver->addClauses(formula);
  ASSERT_EQ(solver->nClauses(), 1ul); 
  ASSERT_TRUE((*clauses)[0]->isDeleted());
}

TEST (CandyAddClauseTestPatterns, unitResolution2) {
  CNFProblem formula;
  formula.readClause(mkLit(1, true));
  formula.readClause(mkLit(1), mkLit(2));
  ClauseDatabase<>* clauses = new ClauseDatabase<>();
  CandyBuilder<> builder { clauses, new Trail() };
  CandySolverInterface* solver = builder.build();
  solver->addClauses(formula);
  ASSERT_EQ(solver->nClauses(), 1ul); 
  ASSERT_TRUE((*clauses)[0]->isDeleted());
}

TEST (CandyAddClauseTestPatterns, unitResolution3) {
  CNFProblem formula;
  formula.readClause(mkLit(1, true));
  formula.readClause({mkLit(1), mkLit(2), mkLit(3)});
  ClauseDatabase<>* clauses = new ClauseDatabase<>();
  CandyBuilder<> builder { clauses, new Trail() };
  CandySolverInterface* solver = builder.build();
  solver->addClauses(formula);
  ASSERT_EQ(solver->nClauses(), 2ul);
  ASSERT_EQ((*clauses)[0]->size(), 3ul);
  ASSERT_TRUE((*clauses)[0]->isDeleted());
  ASSERT_EQ((*clauses)[1]->size(), 2ul);
  ASSERT_FALSE((*clauses)[1]->isDeleted());
}

TEST (CandyAddClauseTestPatterns, removeDuplicates) {
  CNFProblem formula;
  formula.readClause({mkLit(1), mkLit(2), mkLit(1)});
  ClauseDatabase<>* clauses = new ClauseDatabase<>();
  CandyBuilder<> builder { clauses, new Trail() };
  CandySolverInterface* solver = builder.build();
  solver->addClauses(formula);
  ASSERT_EQ((*clauses)[0]->size(), 2ull);
}

TEST(SolverTests, basicIncrementalSolver) {
  CNFProblem problem;
  problem.readClauses({{1_L, 2_L}, {1_L, 1_L}, {2_L, 2_L}});

  Solver<> underTest;
  underTest.addClauses(problem);

  Var assumptionVar = underTest.newVar();
    
  EXPECT_TRUE(l_True == underTest.solve(std::vector<Lit>{mkLit(assumptionVar, 0)}));
  EXPECT_TRUE(l_True == underTest.solve(std::vector<Lit>{mkLit(assumptionVar, 1)}));
}
