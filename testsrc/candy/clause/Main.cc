#include <zlib.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "TestClauseFactory.h"

#include "candy/core/Clause.h"
#include "candy/core/ClauseAllocator.h"

using namespace Candy;
using namespace Glucose;

TestClauseFactory factory;

TEST (CandyClauseTestPatterns, size1) {
    Clause* clause = factory.getClause({lit_Undef});
    ASSERT_EQ(clause->size(), 1);
}

TEST (CandyClauseTestPatterns, size2) {
    Clause* clause = factory.getClause({lit_Undef, lit_Undef});
    ASSERT_EQ(clause->size(), 2);
}

TEST (CandyClauseTestPatterns, size3) {
    Clause* clause = factory.getClause({lit_Undef, lit_Undef, lit_Undef});
    ASSERT_EQ(clause->size(), 3);
}

TEST (CandyClauseHeaderTestPatterns, setLBD) {
    Clause* clause = factory.getClauseWithLBD({lit_Undef}, 1);
    ASSERT_EQ(clause->getLBD(), 1);
    ASSERT_FALSE(clause->isLearnt());
    ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setLearnt) {
    Clause* clause = factory.getClauseLearnt({lit_Undef});
    ASSERT_EQ(clause->getLBD(), 0);
    ASSERT_TRUE(clause->isLearnt());
    ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setDeleted) {
    Clause* clause = factory.getClauseDeleted({lit_Undef});
    ASSERT_EQ(clause->getLBD(), 0);
    ASSERT_FALSE(clause->isLearnt());
    ASSERT_TRUE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setLearntAndLBD) {
    Clause* clause = factory.getClauseLearntWithLBD({lit_Undef}, 255);
    ASSERT_EQ(clause->getLBD(), 255);
    ASSERT_TRUE(clause->isLearnt());
    ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setDeletedAndLBD) {
    Clause* clause = factory.getClauseDeletedWithLBD({lit_Undef}, 255);
    ASSERT_EQ(clause->getLBD(), 255);
    ASSERT_FALSE(clause->isLearnt());
    ASSERT_TRUE(clause->isDeleted());
}

