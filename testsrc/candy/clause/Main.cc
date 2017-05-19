#include <zlib.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "candy/core/Clause.h"
#include "candy/core/ClauseAllocator.h"

using namespace Candy;
using namespace Glucose;

TEST (CandyClauseTestPatterns, size1) {
    ClauseAllocator allocator;
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef});
    ASSERT_EQ(clause->size(), 1);
}

TEST (CandyClauseTestPatterns, size2) {
    ClauseAllocator allocator;
    Clause* clause = new (allocator.allocate(2)) Clause({lit_Undef, lit_Undef});
    ASSERT_EQ(clause->size(), 2);
}

TEST (CandyClauseTestPatterns, size3) {
    ClauseAllocator allocator;
    Clause* clause = new (allocator.allocate(3)) Clause({lit_Undef, lit_Undef, lit_Undef});
    ASSERT_EQ(clause->size(), 3);
}

TEST (CandyClauseHeaderTestPatterns, setLBD) {
    ClauseAllocator allocator;
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef});
    clause->setLBD(1);
    ASSERT_EQ(clause->getLBD(), 1);
    ASSERT_FALSE(clause->isLearnt());
    ASSERT_FALSE(clause->isFrozen());
    ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setFrozen) {
    ClauseAllocator allocator;
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef});
    clause->setFrozen(true);
    ASSERT_EQ(clause->getLBD(), 0);
    ASSERT_FALSE(clause->isLearnt());
    ASSERT_TRUE(clause->isFrozen());
    ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setLearnt) {
    ClauseAllocator allocator;
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef});
    clause->setLearnt(true);
    ASSERT_EQ(clause->getLBD(), 0);
    ASSERT_TRUE(clause->isLearnt());
    ASSERT_FALSE(clause->isFrozen());
    ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setDeleted) {
    ClauseAllocator allocator;
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef});
    clause->setDeleted();
    ASSERT_EQ(clause->getLBD(), 0);
    ASSERT_FALSE(clause->isLearnt());
    ASSERT_FALSE(clause->isFrozen());
    ASSERT_TRUE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setFrozenAndLBD) {
    ClauseAllocator allocator;
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef});
    clause->setFrozen(true);
    clause->setLBD(255);
    ASSERT_EQ(clause->getLBD(), 255);
    ASSERT_FALSE(clause->isLearnt());
    ASSERT_TRUE(clause->isFrozen());
    ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setLearntAndLBD) {
    ClauseAllocator allocator;
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef});
    clause->setLearnt(true);
    clause->setLBD(255);
    ASSERT_EQ(clause->getLBD(), 255);
    ASSERT_TRUE(clause->isLearnt());
    ASSERT_FALSE(clause->isFrozen());
    ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setDeletedAndLBD) {
    ClauseAllocator allocator;
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef});
    clause->setDeleted();
    clause->setLBD(255);
    ASSERT_EQ(clause->getLBD(), 255);
    ASSERT_FALSE(clause->isLearnt());
    ASSERT_FALSE(clause->isFrozen());
    ASSERT_TRUE(clause->isDeleted());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
