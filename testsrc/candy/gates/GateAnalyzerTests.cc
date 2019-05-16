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

namespace Candy {

    typedef std::initializer_list<Lit> ILits;
    typedef std::initializer_list<ILits> IFormula;

    bool equals(Cl clause1, ILits clause2) {
        if (clause1.size() != clause2.size()) {
            return false;
        }
        for (Lit lit : clause2) {
            if(std::find(clause1.begin(), clause1.end(), lit) == clause1.end()) {
                return false;
            }
        }
        return true;
    }

    bool contains(For& super, ILits sub) {
        for (Cl* clause : super) {
            if (equals(*clause, sub)) {
                return true;
            }
        }
        return false;
    }

    bool containsAll(For& super, IFormula sub) {
        for (ILits clause : sub) {
            if (!contains(super, clause)) {
                return false;
            }
        }
        return true;
    }

    void assert_gate(GateAnalyzer& ga, Lit output, bool monotonous, IFormula fwd, IFormula bwd, ILits inputs) {
        Gate g = ga.getResult().getGate(output);
        ASSERT_TRUE(g.isDefined());
        ASSERT_EQ(monotonous, g.hasNonMonotonousParent());
        ASSERT_EQ(fwd.size(), g.getForwardClauses().size());
        ASSERT_EQ(bwd.size(), g.getBackwardClauses().size());
        ASSERT_EQ(inputs.size(), g.getInputs().size());
        ASSERT_TRUE(containsAll(g.getForwardClauses(), fwd));
        ASSERT_TRUE(containsAll(g.getBackwardClauses(), bwd));
        ASSERT_TRUE(equals(g.getInputs(), inputs));
    }

    TEST(GateAnalyzerTest, detectSimpleAnd) {
        CNFProblem problem;
        IFormula simple_and = {{1_L}, {~1_L, 2_L}, {~1_L, 3_L}, {1_L, ~2_L, ~3_L}};
        problem.readClauses(simple_and);
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 1);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_TRUE(equals(ga.getResult().getRootLiterals(), {1_L}));
        assert_gate(ga, 1_L, false, {{~1_L, 2_L}, {~1_L, 3_L}}, {{1_L, ~2_L, ~3_L}}, {2_L, 3_L});
    }

    TEST(GateAnalyzerTest, detectSimpleXor) {
        CNFProblem problem;
        IFormula simple_xor = {{1_L}, {~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}, {1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}};
        problem.readClauses(simple_xor);
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 1);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_TRUE(equals(ga.getResult().getRootLiterals(), {1_L}));
        Gate g = ga.getResult().getGate(1_L);
        assert_gate(ga, 1_L, false, {{~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}}, {{1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}}, {2_L, 3_L, ~2_L, ~3_L});
    }

    TEST(GateAnalyzerTest, detectXorOfAnds) {
        CNFProblem problem;
        IFormula simple_xor = {{1_L}, {~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}, {1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}};
        IFormula simple_and1 = {{~2_L, 4_L}, {~2_L, 5_L}, {2_L, ~4_L, ~5_L}};
        IFormula simple_and2 = {{~3_L, 4_L}, {~3_L, 5_L}, {3_L, ~4_L, ~5_L}};
        problem.readClauses(simple_xor);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 3);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_TRUE(equals(ga.getResult().getRootLiterals(), {1_L}));
        assert_gate(ga, 1_L, false, {{~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}}, {{1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}}, {2_L, 3_L, ~2_L, ~3_L});
        assert_gate(ga, 2_L, true, {{~2_L, 4_L}, {~2_L, 5_L}}, {{2_L, ~4_L, ~5_L}}, {4_L, 5_L});
        assert_gate(ga, 3_L, true, {{~3_L, 4_L}, {~3_L, 5_L}}, {{3_L, ~4_L, ~5_L}}, {4_L, 5_L});
    }

    TEST(GateAnalyzerTest, detectXorOfAndsPlusStuff) {
        CNFProblem problem;
        IFormula simple_xor = {{1_L}, {~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}, {1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}};
        IFormula simple_and1 = {{~2_L, 4_L}, {~2_L, 5_L}, {2_L, ~4_L, ~5_L}};
        IFormula simple_and2 = {{~3_L, 4_L}, {~3_L, 5_L}, {3_L, ~4_L, ~5_L}};
        IFormula stuff = {{6_L, 7_L}};
        problem.readClauses(simple_xor);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        problem.readClauses(stuff);
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 3);
        ASSERT_EQ(ga.getResult().getRoots().size(), 2);
        ASSERT_TRUE(equals(ga.getResult().getRootLiterals(), {1_L, 6_L, 7_L}));
        assert_gate(ga, 1_L, false, {{~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}}, {{1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}}, {2_L, 3_L, ~2_L, ~3_L});
        assert_gate(ga, 2_L, true, {{~2_L, 4_L}, {~2_L, 5_L}}, {{2_L, ~4_L, ~5_L}}, {4_L, 5_L});
        assert_gate(ga, 3_L, true, {{~3_L, 4_L}, {~3_L, 5_L}}, {{3_L, ~4_L, ~5_L}}, {4_L, 5_L});
    }

    TEST(GateAnalyzerTest, detectXorOfAndsPlusStuffAndNormalize) {
        CNFProblem problem;
        IFormula simple_xor = {{1_L}, {~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}, {1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}};
        IFormula simple_and1 = {{~2_L, 4_L}, {~2_L, 5_L}, {2_L, ~4_L, ~5_L}};
        IFormula simple_and2 = {{~3_L, 4_L}, {~3_L, 5_L}, {3_L, ~4_L, ~5_L}};
        IFormula stuff = {{6_L, 7_L}};
        problem.readClauses(simple_xor);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        problem.readClauses(stuff);
        GateAnalyzer ga { problem };
        ga.analyze();
        Lit root = ga.getResult().normalizeRoots();
        ASSERT_EQ(root, 8_L);
        ASSERT_EQ(ga.getResult().getGateCount(), 4);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_EQ(ga.getResult().getRootLiterals().front(), 8_L);
        assert_gate(ga, 1_L, false, {{~1_L, 2_L, 3_L}, {~1_L, ~2_L, ~3_L}}, {{1_L, 2_L, ~3_L}, {1_L, ~2_L, 3_L}}, {2_L, 3_L, ~2_L, ~3_L});
        assert_gate(ga, 2_L, true, {{~2_L, 4_L}, {~2_L, 5_L}}, {{2_L, ~4_L, ~5_L}}, {4_L, 5_L});
        assert_gate(ga, 3_L, true, {{~3_L, 4_L}, {~3_L, 5_L}}, {{3_L, ~4_L, ~5_L}}, {4_L, 5_L});
        assert_gate(ga, 8_L, false, {{~8_L, 1_L}, {~8_L, 6_L, 7_L}}, {}, {1_L, 6_L, 7_L});
    }

    TEST(GateAnalyzerTest, applyPruningToOrOfAnds) {
        CNFProblem problem;
        IFormula simple_or = {{1_L}, {~1_L, 2_L, 3_L}, {1_L, ~2_L}, {1_L, ~3_L}};
        IFormula simple_and1 = {{~2_L, 4_L}, {~2_L, 5_L}, {2_L, ~4_L, ~5_L}};
        IFormula simple_and2 = {{~3_L, 6_L}, {~3_L, 7_L}, {3_L, ~6_L, ~7_L}};
        problem.readClauses(simple_or);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 3);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_TRUE(equals(ga.getResult().getRootLiterals(), {1_L}));
        assert_gate(ga, 1_L, false, {{~1_L, 2_L, 3_L}}, {{1_L, ~2_L}, {1_L, ~3_L}}, {2_L, 3_L});
        assert_gate(ga, 2_L, false, {{~2_L, 4_L}, {~2_L, 5_L}}, {{2_L, ~4_L, ~5_L}}, {4_L, 5_L});
        assert_gate(ga, 3_L, false, {{~3_L, 6_L}, {~3_L, 7_L}}, {{3_L, ~6_L, ~7_L}}, {6_L, 7_L});
        For pp = ga.getPrunedProblem(Cl({1_L, 2_L, ~3_L, 4_L, 5_L, 6_L, 7_L}));
        ASSERT_TRUE(containsAll(pp, simple_or));
        ASSERT_TRUE(containsAll(pp, simple_and1));
        ASSERT_FALSE(contains(pp, {~3_L, 6_L}));
        ASSERT_FALSE(contains(pp, {~3_L, 7_L}));
        ASSERT_FALSE(contains(pp, {3_L, ~6_L, ~7_L}));
    }

    //todo: fix test-case such that also the commented assertions hold
    TEST(GateAnalyzerTest, andOfXorAndPartialXorToTriggerSemanticCheck) {
        CNFProblem problem;
        IFormula simple_and = {{1_L}, {~1_L, 2_L}, {~1_L, 3_L}, {1_L, ~2_L, ~3_L}};
        IFormula simple_xor1 = {{~2_L, 4_L, 5_L}, {~2_L, ~4_L, ~5_L}, {2_L, 4_L, ~5_L}, {2_L, ~4_L, 5_L}};
        IFormula simple_eq = {{~3_L, 6_L, ~7_L}, {~3_L, ~6_L, 7_L}, {3_L, 6_L, 7_L}, {3_L, ~6_L, ~7_L}};
        IFormula simple_xor2 = {{~6_L, 4_L, 5_L}, {~6_L, ~4_L, ~5_L}};
        problem.readClauses(simple_and);
        problem.readClauses(simple_xor1);
        problem.readClauses(simple_eq);
        problem.readClauses(simple_xor2);
        GateAnalyzer ga { problem, 0, 3, true, true, true };
        ga.analyze();
//        ASSERT_EQ(ga.getGateCount(), 4);
//        ASSERT_EQ(ga.getRoots().size(), 1);
//        ASSERT_TRUE(equals(ga.getRootLiterals(), {1_L}));
        assert_gate(ga, 1_L, false, {{~1_L, 2_L}, {~1_L, 3_L}}, {{1_L, ~2_L, ~3_L}}, {2_L, 3_L});
        assert_gate(ga, 2_L, false, {{~2_L, 4_L, 5_L}, {~2_L, ~4_L, ~5_L}}, {{2_L, 4_L, ~5_L}, {2_L, ~4_L, 5_L}}, {4_L, 5_L, ~4_L, ~5_L});
        assert_gate(ga, 3_L, false, {{~3_L, 6_L, ~7_L}, {~3_L, ~6_L, 7_L}}, {{3_L, 6_L, 7_L}, {3_L, ~6_L, ~7_L}}, {6_L, 7_L, ~6_L, ~7_L});
//        assert_gate(ga, 6_L, false, {{~6_L, 4_L, 5_L}, {~6_L, ~4_L, ~5_L}}, {}, {4_L, 5_L, ~4_L, ~5_L});
    }
}
