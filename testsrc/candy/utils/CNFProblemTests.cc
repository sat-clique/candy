#include <zlib.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "candy/testutils/TestUtils.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/CNFProblem.h"

using namespace Candy;

TEST (CNFProblemTestPatterns, clausePersistance) {
    CNFProblem problem;
    problem.readClause({Lit(0, 0), Lit(1, 0)});
    EXPECT_TRUE(containsClause(problem, {Lit(0, 0), Lit(1, 0)}));
}

// CNFProblem does take care of clause-sanitation again:
TEST (CNFProblemTestPatterns, redundandLiteralRemoval) {
    CNFProblem problem;
    problem.readClause({Lit(0, 0), Lit(1, 0)});
    EXPECT_TRUE(containsClause(problem, {Lit(0), Lit(1, 0)}));
}

TEST (CNFProblemTestPatterns, tautologicClauseRemoval) {
    CNFProblem problem;
    problem.readClause({Lit(0, 0), Lit(1, 0), Lit(0, 1)});
    ASSERT_EQ(problem.nClauses(), 0);
}

