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

    typedef std::initializer_list<Lit> IClause;
    typedef std::initializer_list<IClause> IFormula;

    bool contains(For& super, IClause sub) {
        bool found = false;
        for (Cl* clause : super) {
            if (clause->size() != sub.size()) {
                continue;
            }
            found = true;
            for (Lit lit : sub) {
                if(std::find(clause->begin(), clause->end(), lit) == clause->end()) {
                    found = false;
                    break;
                }
            }
            if (found) {
                return true;
            }
        }
        return false;
    }

    bool containsAll(For& super, IFormula sub) {
        for (IClause clause : sub) {
            contains(super, clause);
        }
    }

    TEST(GateAnalyzerTest, detectSimpleAnd) {
        CNFProblem problem;
        IFormula simple_and = {{1_L}, {~1_L, 2_L}, {~1_L, 3_L}, {1_L, ~2_L, ~3_L}};
        problem.readClauses(simple_and);
        GateAnalyzer ga(problem);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), 1);
        Gate g = ga.getGate(1_L);
        ASSERT_TRUE(g.isDefined());
        ASSERT_FALSE(g.hasNonMonotonousParent());
        ASSERT_EQ(g.getForwardClauses().size(), 2);
        ASSERT_EQ(g.getBackwardClauses().size(), 1);
        ASSERT_EQ(g.getInputs().size(), 2);
    }

    TEST(GateAnalyzerTest, detectSimpleXor) {
        CNFProblem problem;
        IFormula simple_xor = {{1_L}, {~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}, {1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}};
        problem.readClauses(simple_xor);
        GateAnalyzer ga(problem);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), 1);
        Gate g = ga.getGate(1_L);
        ASSERT_TRUE(g.isDefined());
        ASSERT_FALSE(g.hasNonMonotonousParent());
        ASSERT_EQ(g.getForwardClauses().size(), 2);
        ASSERT_EQ(g.getBackwardClauses().size(), 2);
        ASSERT_EQ(g.getInputs().size(), 4);
    }

    TEST(GateAnalyzerTest, detectXorOfAnds) {
        CNFProblem problem;
        IFormula simple_xor = {{1_L}, {~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}, {1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}};
        IFormula simple_and1 = {{2_L}, {~2_L, 4_L}, {~2_L, 5_L}, {2_L, ~4_L, ~5_L}};
        IFormula simple_and2 = {{3_L}, {~3_L, 4_L}, {~3_L, 5_L}, {3_L, ~4_L, ~5_L}};
        problem.readClauses(simple_xor);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        GateAnalyzer ga(problem);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), 3);
        Gate g = ga.getGate(1_L);
        ASSERT_TRUE(g.isDefined());
        ASSERT_FALSE(g.hasNonMonotonousParent());
        ASSERT_EQ(g.getForwardClauses().size(), 2);
        ASSERT_EQ(g.getBackwardClauses().size(), 2);
        ASSERT_EQ(g.getInputs().size(), 4);
        g = ga.getGate(2_L);
        ASSERT_TRUE(g.isDefined());
        ASSERT_TRUE(g.hasNonMonotonousParent());
        ASSERT_EQ(g.getForwardClauses().size(), 2);
        ASSERT_EQ(g.getBackwardClauses().size(), 1);
        ASSERT_EQ(g.getInputs().size(), 2);
        g = ga.getGate(3_L);
        ASSERT_TRUE(g.isDefined());
        ASSERT_TRUE(g.hasNonMonotonousParent());
        ASSERT_EQ(g.getForwardClauses().size(), 2);
        ASSERT_EQ(g.getBackwardClauses().size(), 1);
        ASSERT_EQ(g.getInputs().size(), 2);
    }
}
