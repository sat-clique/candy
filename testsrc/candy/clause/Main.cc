#include <zlib.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "candy/core/Clause.h"
#include "candy/core/ClauseAllocator.h"

using namespace Candy;

ClauseAllocator allocator;

TEST (CandyClauseTestPatterns, size1) {
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef});
    ASSERT_EQ(clause->size(), 1);
}

TEST (CandyClauseTestPatterns, size2) {
    Clause* clause = new (allocator.allocate(2)) Clause({lit_Undef, lit_Undef});
    ASSERT_EQ(clause->size(), 2);
}

TEST (CandyClauseTestPatterns, size3) {
    Clause* clause = new (allocator.allocate(2)) Clause({lit_Undef, lit_Undef, lit_Undef});
    ASSERT_EQ(clause->size(), 3);
}

TEST (CandyClauseHeaderTestPatterns, learnt) {
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef}, 1);
    ASSERT_EQ(clause->getLBD(), 1);
    ASSERT_TRUE(clause->isLearnt());
    ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, deleted) {
    Clause* clause = new (allocator.allocate(1)) Clause({lit_Undef}, std::numeric_limits<uint16_t>::max());
    ASSERT_EQ(clause->getLBD(), std::numeric_limits<uint16_t>::max());
    ASSERT_TRUE(clause->isLearnt());
    ASSERT_TRUE(clause->isDeleted());
}
