/* Copyright (c) 2017 Markus Iser (github.com/udopia)

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 Except as contained in this notice, the name(s) of the above copyright holders
 shall not be used in advertising or otherwise to promote the sale, use or
 other dealings in this Software without prior written authorization.

 */

#include <gtest/gtest.h>

#include <candy/core/SolverTypes.h>
#include <candy/gates/GateDFSTraversal.h>
#include <candy/testutils/TestGateStructure.h>

#include <iostream>
#include <algorithm>

/*
    GateAnalyzer(CNFProblem& dimacs, int tries = 0,
            bool patterns = true, bool semantic = true, bool holistic = false,
            bool lookahead = false, bool intensify = true, int lookahead_threshold = 10,
            unsigned int conflict_budget = 0,
            std::chrono::milliseconds timeout = std::chrono::milliseconds{0})
 */

namespace Candy {

    typedef std::initializer_list<std::initializer_list<Lit>> formula;


    TEST(GateAnalyzerTest, detectSimpleAnd) {
        CNFProblem problem;
        formula simple_and = {{1_L}, {~1_L, 2_L}, {~1_L, 3_L}, {1_L, ~2_L, ~3_L}};
        problem.readClauses(simple_and);
        GateAnalyzer ga(problem);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), 1);
        Gate g = ga.getGate(1_L);
        ASSERT_TRUE(g.isDefined());
        ASSERT_EQ(g.getForwardClauses().size(), 2);
        ASSERT_EQ(g.getBackwardClauses().size(), 1);
        ASSERT_EQ(g.getInputs().size(), 2);
    }

    TEST(GateAnalyzerTest, detectSimpleXor) {
        CNFProblem problem;
        formula simple_xor = {{1_L}, {~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}, {1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}};
        problem.readClauses(simple_xor);
        GateAnalyzer ga(problem);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), 1);
        Gate g = ga.getGate(1_L);
        ASSERT_TRUE(g.isDefined());
        ASSERT_EQ(g.getForwardClauses().size(), 2);
        ASSERT_EQ(g.getBackwardClauses().size(), 2);
        ASSERT_EQ(g.getInputs().size(), 4);
    }
}
