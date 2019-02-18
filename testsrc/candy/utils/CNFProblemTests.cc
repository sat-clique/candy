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
    problem.readClause({mkLit(0, 0), mkLit(1, 0)});
    EXPECT_TRUE(containsClause(problem, {mkLit(0, 0), mkLit(1, 0)}));
}

// CNFProblem does take care of clause-sanitation again:
TEST (CNFProblemTestPatterns, redundandLiteralRemoval) {
    CNFProblem problem;
    problem.readClause({mkLit(0, 0), mkLit(1, 0)});
    EXPECT_TRUE(containsClause(problem, {mkLit(0), mkLit(1, 0)}));
}

TEST (CNFProblemTestPatterns, tautologicClauseRemoval) {
    CNFProblem problem;
    problem.readClause({mkLit(0, 0), mkLit(1, 0), mkLit(0, 1)});
    ASSERT_EQ(problem.nClauses(), 0);
}

