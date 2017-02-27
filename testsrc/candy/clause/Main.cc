#include <zlib.h>
#include <sys/resource.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "core/Clause.h"

using namespace Candy;
using namespace Glucose;

TEST (CandyClauseTestPatterns, size1) {
  Clause* clause = new (1) Clause({lit_Undef});
  ASSERT_EQ(clause->size(), 1);
}

TEST (CandyClauseTestPatterns, size2) {
  Clause* clause = new (2) Clause({lit_Undef, lit_Undef});
  ASSERT_EQ(clause->size(), 2);
}

TEST (CandyClauseTestPatterns, size3) {
  Clause* clause = new (3) Clause({lit_Undef, lit_Undef, lit_Undef});
  ASSERT_EQ(clause->size(), 3);
}

TEST (CandyClauseHeaderTestPatterns, setLBD) {
  Clause* clause = new (1) Clause({lit_Undef});
  clause->setLBD(1);
  ASSERT_EQ(clause->getLBD(), 1);
  ASSERT_FALSE(clause->isLearnt());
  ASSERT_FALSE(clause->isFrozen());
  ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setFrozen) {
  Clause* clause = new (1) Clause({lit_Undef});
  clause->setFrozen(true);
  ASSERT_EQ(clause->getLBD(), 0);
  ASSERT_FALSE(clause->isLearnt());
  ASSERT_TRUE(clause->isFrozen());
  ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setLearnt) {
  Clause* clause = new (1) Clause({lit_Undef});
  clause->setLearnt(true);
  ASSERT_EQ(clause->getLBD(), 0);
  ASSERT_TRUE(clause->isLearnt());
  ASSERT_FALSE(clause->isFrozen());
  ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setDeleted) {
  Clause* clause = new (1) Clause({lit_Undef});
  clause->setDeleted();
  ASSERT_EQ(clause->getLBD(), 0);
  ASSERT_FALSE(clause->isLearnt());
  ASSERT_FALSE(clause->isFrozen());
  ASSERT_TRUE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setFrozenAndLBD) {
  Clause* clause = new (1) Clause({lit_Undef});
  clause->setFrozen(true);
  clause->setLBD(255);
  ASSERT_EQ(clause->getLBD(), 255);
  ASSERT_FALSE(clause->isLearnt());
  ASSERT_TRUE(clause->isFrozen());
  ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setLearntAndLBD) {
  Clause* clause = new (1) Clause({lit_Undef});
  clause->setLearnt(true);
  clause->setLBD(255);
  ASSERT_EQ(clause->getLBD(), 255);
  ASSERT_TRUE(clause->isLearnt());
  ASSERT_FALSE(clause->isFrozen());
  ASSERT_FALSE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, setDeletedAndLBD) {
  Clause* clause = new (1) Clause({lit_Undef});
  clause->setDeleted();
  clause->setLBD(255);
  ASSERT_EQ(clause->getLBD(), 255);
  ASSERT_FALSE(clause->isLearnt());
  ASSERT_FALSE(clause->isFrozen());
  ASSERT_TRUE(clause->isDeleted());
}

TEST (CandyClauseHeaderTestPatterns, clauseLTwithLBD) {
  Clause* clause1 = new (1) Clause({lit_Undef});
  Clause* clause2 = new (1) Clause({lit_Undef});
  clause1->setLBD(1024);
  clause2->setLBD(256);
  ASSERT_TRUE(*clause1 < *clause2);
}

TEST (CandyClauseHeaderTestPatterns, clauseLTwithLBDAndFrozen) {
  Clause* clause1 = new (1) Clause({lit_Undef});
  Clause* clause2 = new (1) Clause({lit_Undef});
  clause1->setLBD(1024);
  clause2->setLBD(256);
  clause1->setFrozen(true);
  ASSERT_FALSE(*clause1 < *clause2);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
