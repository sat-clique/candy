#include <zlib.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "gtest/gtest.h"

#include "candy/testutils/TestUtils.h"
#include "candy/core/SolverTypes.h"
#include "candy/mtl/State.h"

using namespace Candy;

TEST (StateTestPatterns, initializationTest) {
	State<uint32_t, 3> state(3);
	EXPECT_EQ(state[0], 0);
	EXPECT_EQ(state[1], 0);
	EXPECT_EQ(state[2], 0);
}

TEST (StateTestPatterns, setWorks) {
	State<uint32_t, 3> state(3);
	state.set(1, 1);
	state.set(2, 2);
	EXPECT_EQ(state[0], 0);
	EXPECT_EQ(state[1], 1);
	EXPECT_EQ(state[2], 2);
	state.set(1, 2);
	state.set(2, 0);
	EXPECT_EQ(state[0], 0);
	EXPECT_EQ(state[1], 2);
	EXPECT_EQ(state[2], 0);
}

TEST (StatePatterns, clearWorks) {
	State<uint32_t, 3> state(3);
	state.set(1, 1);
	state.set(2, 2);
	EXPECT_EQ(state[0], 0);
	EXPECT_EQ(state[1], 1);
	EXPECT_EQ(state[2], 2);
	state.clear();
	EXPECT_EQ(state[0], 0);
	EXPECT_EQ(state[1], 0);
	EXPECT_EQ(state[2], 0);
}

TEST (StateTestPatterns, overflowTest) {
	State<uint8_t, 3> state(1);
	for (int i = 0; i < 257; i++) {
		EXPECT_EQ(state[0], 0);
		state.set(0, 1);
		EXPECT_EQ(state[0], 1);
		state.set(0, 2);
		EXPECT_EQ(state[0], 2);
		state.clear();
	}
}
