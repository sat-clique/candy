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
            return false;
        }
        for (auto it = begin2; it != end2; it++) {
            if(std::find(begin1, end1, *it) == end1) {
                return false;
            }
        }
        return true;
    }

    template<typename Iterator> 
    bool contains(For& super, Iterator begin, Iterator end) {
        for (Cl* clause : super) {
            if (equals(clause->begin(), clause->end(), begin, end)) {
                return true;
            }
        }
        return false;
    }

    bool containsAll(For& super, For& sub) {
        for (Cl* clause : sub) {
            if (!contains(super, clause->begin(), clause->end())) {
                return false;
            }
        }
        return true;
    }

    void assert_gate(GateAnalyzer& ga, Lit output, bool not_monotonous, For clauses, std::initializer_list<Lit> inputs) {
        ASSERT_TRUE(ga.getResult().isGateOutput(output));
        Gate g = ga.getResult().getGate(output); 
        ASSERT_EQ(not_monotonous, g.hasNonMonotonicParent());
        ASSERT_EQ(clauses.size(), g.fwd.size() + g.bwd.size());
        ASSERT_EQ(inputs.size(), g.inp.size());
        ASSERT_TRUE(equals(g.inp.cbegin(), g.inp.cend(), inputs.begin(), inputs.end())); 
        ASSERT_TRUE(containsAll(clauses, g.fwd));
        ASSERT_TRUE(containsAll(clauses, g.bwd));
    }

    TEST(GateAnalyzerTest, detectSimpleAnd) {
        CNFProblem problem;
        For simple_and = GateBuilder::and_gate(1_L, 2_L, 3_L);
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
        For simple_xor = GateBuilder::xor_gate(1_L, 2_L, 3_L);
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
        For simple_xor = GateBuilder::xor_gate(1_L, 2_L, 3_L);
        For simple_and1 = GateBuilder::and_gate(2_L, 4_L, 5_L);
        For simple_and2 = GateBuilder::and_gate(3_L, 4_L, 5_L);
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

    TEST(GateAnalyzerTest, detectXorOfAndsWithRemainder) {
        CNFProblem problem;
        For simple_xor = GateBuilder::xor_gate(1_L, 2_L, 3_L);
        For simple_and1 = GateBuilder::and_gate(2_L, 4_L, 5_L);
        For simple_and2 = GateBuilder::and_gate(3_L, 4_L, 5_L);
        problem.readClause({1_L});
        problem.readClauses(simple_xor);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        problem.readClause({6_L, 7_L});
        GateAnalyzer ga { problem };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 3);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        Cl clause ({1_L});
        Cl roots = ga.getResult().getRootLiterals();
        ASSERT_TRUE(equals(roots.begin(), roots.end(), clause.begin(), clause.end()));
        assert_gate(ga, 1_L, false, simple_xor, {2_L, 3_L, ~2_L, ~3_L});
        assert_gate(ga, 2_L, true, simple_and1, {4_L, 5_L});
        assert_gate(ga, 3_L, true, simple_and2, {4_L, 5_L});
    }

    TEST(GateAnalyzerTest, normalizeIncludesRemainder) {
        CNFProblem problem;
        For simple_xor = GateBuilder::xor_gate(1_L, 2_L, 3_L);
        For simple_and1 = GateBuilder::and_gate(2_L, 4_L, 5_L);
        For simple_and2 = GateBuilder::and_gate(3_L, 4_L, 5_L);
        problem.readClause({1_L});
        problem.readClause({6_L, 7_L});
        problem.readClauses(simple_xor);
        problem.readClauses(simple_and1);
        problem.readClauses(simple_and2);
        GateAnalyzer ga { problem, GateRecognitionMethod::Patterns, ClauseSelectionMethod::UnitClausesThenMaximalLiterals, 2 };
        ga.analyze();
        ga.getResult().normalizeRoots();
        Lit root = ga.getResult().getRoot();
        ASSERT_EQ(root, 8_L);
        ASSERT_EQ(ga.getResult().getGateCount(), 4);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_EQ(ga.getResult().getRootLiterals().front(), 8_L);
        For rootgate ({new Cl({~8_L, 1_L}), new Cl({~8_L, 6_L, 7_L})});
        assert_gate(ga, 8_L, false, rootgate, {1_L, 6_L, 7_L});
    }

    TEST(GateAnalyzerTest, pruningWorks) {
        CNFProblem problem;
        For simple_or = GateBuilder::or_gate(1_L, 2_L, 3_L);
        For simple_and1 = GateBuilder::and_gate(2_L, 4_L, 5_L);
        For simple_and2 = GateBuilder::and_gate(3_L, 6_L, 7_L);
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
        For pruned = ga.getResult().getPrunedProblem(Cl({1_L, 2_L, ~3_L, 4_L, 5_L, 6_L, 7_L}));
        ASSERT_TRUE(containsAll(pruned, simple_or));
        ASSERT_TRUE(containsAll(pruned, simple_and1));
        For f ({new Cl({~3_L, 6_L}), new Cl({~3_L, 7_L}), new Cl({3_L, ~6_L, ~7_L})});
        ASSERT_FALSE(containsAll(pruned, f));
    }

    //todo: fix test-case such that also the commented assertions hold
    TEST(GateAnalyzerTest, andOfXorAndPartialXorToTriggerSemanticCheck) {
        CNFProblem problem;
        For simple_and = GateBuilder::and_gate(1_L, 2_L, 3_L);
        For simple_xor1 = GateBuilder::xor_gate(2_L, 4_L, 5_L);
        For simple_eq = GateBuilder::xor_gate(3_L, 6_L, ~7_L);
        For simple_or = GateBuilder::or_gate(6_L, 4_L, 5_L, true);
        problem.readClause({1_L});
        problem.readClauses(simple_and);
        problem.readClauses(simple_xor1);
        problem.readClauses(simple_eq);
        problem.readClauses(simple_or);
        GateAnalyzer ga { problem, GateRecognitionMethod::Patterns };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 4);
        ASSERT_EQ(ga.getResult().getRoots().size(), 1);
        ASSERT_EQ(ga.getResult().getRootLiterals().front(), 1_L);
        assert_gate(ga, 1_L, false, simple_and, {2_L, 3_L});
        assert_gate(ga, 2_L, false, simple_xor1, {4_L, 5_L, ~4_L, ~5_L});
        assert_gate(ga, 3_L, false, simple_eq, {6_L, 7_L, ~6_L, ~7_L});
        assert_gate(ga, 6_L, true, simple_or, {4_L, 5_L});
    }
}
