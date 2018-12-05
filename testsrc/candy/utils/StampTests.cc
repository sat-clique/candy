#include <zlib.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "candy/testutils/TestUtils.h"
#include "candy/core/SolverTypes.h"
#include "candy/mtl/Stamp.h"

using namespace Candy;
using namespace Glucose;

TEST (StampTestPatterns, initializationTest) {
	Stamp<uint32_t> stamp(3);
	EXPECT_TRUE(stamp.isStamped(0));
	EXPECT_TRUE(stamp.isStamped(1));
	EXPECT_TRUE(stamp.isStamped(2));
	stamp.clear();
	EXPECT_FALSE(stamp.isStamped(0));
	EXPECT_FALSE(stamp.isStamped(1));
	EXPECT_FALSE(stamp.isStamped(2));
}

TEST (StampTestPatterns, stampWorks) {
	Stamp<uint32_t> stamp(3);
	stamp.clear();
	stamp.set(1);
	EXPECT_FALSE(stamp.isStamped(0));
	EXPECT_TRUE(stamp.isStamped(1));
	EXPECT_FALSE(stamp.isStamped(2));
}

TEST (StampPatterns, unstampWorks) {
	Stamp<uint32_t> stamp(3);
	stamp.clear();
	stamp.set(1);
	stamp.unset(1);
	EXPECT_FALSE(stamp.isStamped(0));
	EXPECT_FALSE(stamp.isStamped(1));
	EXPECT_FALSE(stamp.isStamped(2));
}

TEST (StampTestPatterns, overflowTest) {
	Stamp<bool> stamp(1);
	EXPECT_TRUE(stamp.isStamped(0));
	stamp.clear();
	EXPECT_FALSE(stamp.isStamped(0));
	stamp.set(0);
	EXPECT_TRUE(stamp.isStamped(0));
	stamp.clear();
	EXPECT_FALSE(stamp.isStamped(0));
	stamp.set(0);
	EXPECT_TRUE(stamp.isStamped(0));
}

TEST (StampTestPatterns, rigidOverflowTest) {
	Stamp<uint8_t> stamp(1);
	EXPECT_TRUE(stamp.isStamped(0));
	for (int i = 0; i < 258; i++) {
		stamp.clear();
		EXPECT_FALSE(stamp.isStamped(0));
		stamp.set(0);
		EXPECT_TRUE(stamp.isStamped(0));
	}
}
