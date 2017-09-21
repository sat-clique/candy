#include <zlib.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "candy/testutils/TestUtils.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/Stamp.h"

using namespace Candy;
using namespace Glucose;

TEST (StampTestPatterns, initializationTest) {
	Stamp<uint32_t> stamp(3);
	EXPECT_TRUE(stamp.isStamped(0));
	EXPECT_TRUE(stamp.isStamped(1));
	EXPECT_TRUE(stamp.isStamped(2));
	stamp.clearStamped();
	EXPECT_FALSE(stamp.isStamped(0));
	EXPECT_FALSE(stamp.isStamped(1));
	EXPECT_FALSE(stamp.isStamped(2));
}

TEST (StampTestPatterns, stampWorks) {
	Stamp<uint32_t> stamp(3);
	stamp.clearStamped();
	stamp.setStamped(1);
	EXPECT_FALSE(stamp.isStamped(0));
	EXPECT_TRUE(stamp.isStamped(1));
	EXPECT_FALSE(stamp.isStamped(2));
}

TEST (StampPatterns, unstampWorks) {
	Stamp<uint32_t> stamp(3);
	stamp.clearStamped();
	stamp.setStamped(1);
	stamp.unsetStamped(1);
	EXPECT_FALSE(stamp.isStamped(0));
	EXPECT_FALSE(stamp.isStamped(1));
	EXPECT_FALSE(stamp.isStamped(2));
}

TEST (StampTestPatterns, overflowTest) {
	Stamp<bool> stamp(1);
	EXPECT_TRUE(stamp.isStamped(0));
	stamp.clearStamped();
	EXPECT_FALSE(stamp.isStamped(0));
	stamp.setStamped(0);
	EXPECT_TRUE(stamp.isStamped(0));
	stamp.clearStamped();
	EXPECT_FALSE(stamp.isStamped(0));
	stamp.setStamped(0);
	EXPECT_TRUE(stamp.isStamped(0));
}
