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
#include <candy/gates/GateBuilder.h>
#include <candy/gates/GateAnalyzer.h>

#include <iostream>
#include <algorithm>

namespace Candy {

    template<typename Iterator, typename Iterator2> 
    bool equals(Iterator begin1, Iterator end1, Iterator2 begin2, Iterator2 end2) {
        if (std::distance(begin1, end1) != std::distance(begin2, end2)) {
            std::cout << "not equal " << std::distance(begin1, end1) << " " << std::distance(begin2, end2) << std::endl;
            return false;
        }
        for (auto it = begin2; it != end2; it++) {
            if(std::find(begin1, end1, *it) == end1) {
                std::cout << *it << "not found" << std::endl;
                return false;
            }
        }
        return true;
    }

    template<typename Iterator> 
    bool contains(Formula super, Iterator begin, Iterator end) {
        for (Cl& clause : super) {
            if (equals(clause.begin(), clause.end(), begin, end)) {
                return true;
            }
        }
        return false;
    }

    bool containsAll(Formula super, For& sub) {
        for (Cl* clause : sub) {
            if (!contains(super, clause->begin(), clause->end())) {
                return false;
            }
        }
        return true;
    }

    bool containsAll(Formula super, Formula sub) {
        for (Cl& clause : sub) {
            if (!contains(super, clause.begin(), clause.end())) {
                return false;
            }
        }
        return true;
    }

    void assert_gate(GateAnalyzer& ga, Lit output, bool monotonous, Formula clauses, std::initializer_list<Lit> inputs) {
        Gate g = ga.getResult().getGate(output); 
        ASSERT_TRUE(g.isDefined());
        ASSERT_EQ(monotonous, g.hasNonMonotonousParent());
        ASSERT_EQ(clauses.size(), g.getForwardClauses().size() + g.getBackwardClauses().size());
        ASSERT_EQ(inputs.size(), g.getInputs().size());
        ASSERT_TRUE(equals(g.getInputs().cbegin(), g.getInputs().cend(), inputs.begin(), inputs.end())); 
        std::cout << "fwd " << g.getForwardClauses() << std::endl;
        ASSERT_TRUE(containsAll(clauses, g.getForwardClauses()));
        ASSERT_TRUE(containsAll(clauses, g.getBackwardClauses()));
    }

    TEST(GateAnalyzerTest, detectSimpleAnd) {
        CNFProblem problem;
        Formula simple_and = GateBuilder::and_gate(1_L, 2_L, 3_L);
        problem.readClause({1_L});
        problem.readClauses(simple_and);
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 1);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_EQ(ga.getResult().getRootLiterals().front(), 1_L);
        assert_gate(ga, 1_L, false, simple_and, {2_L, 3_L});
    }

    TEST(GateAnalyzerTest, detectSimpleXor) {
        CNFProblem problem;
        Formula simple_xor = GateBuilder::xor_gate(1_L, 2_L, 3_L);
        problem.readClause({1_L});
        problem.readClauses(simple_xor);
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 1);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_EQ(ga.getResult().getRootLiterals().front(), 1_L);
        Gate g = ga.getResult().getGate(1_L);
        assert_gate(ga, 1_L, false, simple_xor, {2_L, 3_L, ~2_L, ~3_L});
    }

    TEST(GateAnalyzerTest, detectXorOfAnds) {
        CNFProblem problem;
        Formula simple_xor = GateBuilder::xor_gate(1_L, 2_L, 3_L);
        Formula simple_and1 = GateBuilder::and_gate(2_L, 4_L, 5_L);
        Formula simple_and2 = GateBuilder::and_gate(3_L, 4_L, 5_L);
        problem.readClause({1_L});
        problem.readClauses(simple_xor);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 3);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_EQ(ga.getResult().getRootLiterals().front(), 1_L);
        assert_gate(ga, 1_L, false, simple_xor, {2_L, 3_L, ~2_L, ~3_L});
        assert_gate(ga, 2_L, true, simple_and1, {4_L, 5_L});
        assert_gate(ga, 3_L, true, simple_and2, {4_L, 5_L});
    }

    TEST(GateAnalyzerTest, detectXorOfAndsPlusStuff) {
        CNFProblem problem;
        Formula simple_xor = GateBuilder::xor_gate(1_L, 2_L, 3_L);
        Formula simple_and1 = GateBuilder::and_gate(2_L, 4_L, 5_L);
        Formula simple_and2 = GateBuilder::and_gate(3_L, 4_L, 5_L);
        problem.readClause({1_L});
        problem.readClauses(simple_xor);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        problem.readClause({6_L, 7_L});
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 3);
        ASSERT_EQ(ga.getResult().getRoots().size(), 2);
        Cl clause ({1_L, 6_L, 7_L});
        Cl roots = ga.getResult().getRootLiterals();
        std::cout << roots << std::endl;
        std::cout << clause << std::endl;
        ASSERT_TRUE(equals(roots.begin(), roots.end(), clause.begin(), clause.end()));
        assert_gate(ga, 1_L, false, simple_xor, {2_L, 3_L, ~2_L, ~3_L});
        assert_gate(ga, 2_L, true, simple_and1, {4_L, 5_L});
        assert_gate(ga, 3_L, true, simple_and2, {4_L, 5_L});
    }

    TEST(GateAnalyzerTest, normalizeIncludesRemainder) {
        CNFProblem problem;
        Formula simple_xor = GateBuilder::xor_gate(1_L, 2_L, 3_L);
        Formula simple_and1 = GateBuilder::and_gate(2_L, 4_L, 5_L);
        Formula simple_and2 = GateBuilder::and_gate(3_L, 4_L, 5_L);
        problem.readClause({1_L});
        problem.readClause({6_L, 7_L});
        problem.readClauses(simple_xor);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        GateAnalyzer ga { problem };
        ga.analyze();
        ga.getResult().normalizeRoots();
        Lit root = ga.getResult().getRoot();
        ASSERT_EQ(root, 8_L);
        ASSERT_EQ(ga.getResult().getGateCount(), 4);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_EQ(ga.getResult().getRootLiterals().front(), 8_L);
        Formula rootgate ({{~8_L, 1_L}, {~8_L, 6_L, 7_L}});
        assert_gate(ga, 8_L, false, rootgate, {1_L, 6_L, 7_L});
    }

    TEST(GateAnalyzerTest, pruningWorks) {
        CNFProblem problem;
        Formula simple_or = GateBuilder::or_gate(1_L, 2_L, 3_L);
        Formula simple_and1 = GateBuilder::and_gate(2_L, 4_L, 5_L);
        Formula simple_and2 = GateBuilder::and_gate(3_L, 6_L, 7_L);
        problem.readClause({1_L});
        problem.readClauses(simple_or);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 3);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_EQ(ga.getResult().getRootLiterals().front(), 1_L);
        assert_gate(ga, 1_L, false, simple_or, {2_L, 3_L});
        assert_gate(ga, 2_L, false, simple_and1, {4_L, 5_L});
        assert_gate(ga, 3_L, false, simple_and2, {6_L, 7_L});
        Formula pruned = ga.getResult().getPrunedProblem(Cl({1_L, 2_L, ~3_L, 4_L, 5_L, 6_L, 7_L}));
        ASSERT_TRUE(containsAll(pruned, simple_or));
        ASSERT_TRUE(containsAll(pruned, simple_and1));
        Formula f ({Cl({~3_L, 6_L}), Cl({~3_L, 7_L}), Cl({3_L, ~6_L, ~7_L})});
        ASSERT_FALSE(containsAll(pruned, f));
    }

    //todo: fix test-case such that also the commented assertions hold
    TEST(GateAnalyzerTest, andOfXorAndPartialXorToTriggerSemanticCheck) {
        CNFProblem problem;
        Formula simple_and = GateBuilder::and_gate(1_L, 2_L, 3_L);
        Formula simple_xor1 = GateBuilder::xor_gate(2_L, 4_L, 5_L);
        Formula simple_eq = GateBuilder::xor_gate(3_L, 6_L, ~7_L);
        Formula simple_xor2 = GateBuilder::xor_gate(6_L, 4_L, 5_L);
        problem.readClause({1_L});
        problem.readClauses(simple_and);
        problem.readClauses(simple_xor1);
        problem.readClauses(simple_eq);
        problem.readClauses(simple_xor2);
        GateAnalyzer ga { problem, 0, 3, true, true, true };
        ga.analyze();
//        ASSERT_EQ(ga.getGateCount(), 4);
//        ASSERT_EQ(ga.getRoots().size(), 1);
//        ASSERT_TRUE(equals(ga.getRootLiterals(), {1_L}));
        assert_gate(ga, 1_L, false, simple_and, {2_L, 3_L});
        assert_gate(ga, 2_L, false, simple_xor1, {4_L, 5_L, ~4_L, ~5_L});
        assert_gate(ga, 3_L, false, simple_eq, {6_L, 7_L, ~6_L, ~7_L});
//        assert_gate(ga, 6_L, false, {{~6_L, 4_L, 5_L}, {~6_L, ~4_L, ~5_L}}, {}, {4_L, 5_L, ~4_L, ~5_L});
    }
}
