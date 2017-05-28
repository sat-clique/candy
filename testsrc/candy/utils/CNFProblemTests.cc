#include <zlib.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "candy/testutils/TestUtils.h"
#include "candy/core/SolverTypes.h"
#include "candy/utils/CNFProblem.h"

using namespace Candy;
using namespace Glucose;

TEST (CNFProblemTestPatterns, clausePersistance) {
    CNFProblem problem;
    problem.readClause({mkLit(0, 0), mkLit(1, 0)});
    EXPECT_TRUE(containsClause(problem.getProblem(), {mkLit(0, 0), mkLit(1, 0)}));
}

TEST (CNFProblemTestPatterns, redundandLiteralRemoval) {
    CNFProblem problem;
    problem.readClause({mkLit(0, 0), mkLit(1, 0), mkLit(0, 0)});
    EXPECT_TRUE(containsClause(problem.getProblem(), {mkLit(0, 0), mkLit(1, 0)}));
}

TEST (CNFProblemTestPatterns, tautologicClauseRemoval) {
    CNFProblem problem;
    problem.readClause({mkLit(0, 0), mkLit(1, 0), mkLit(0, 1)});
    ASSERT_EQ(problem.getProblem().size(), 0);
}

