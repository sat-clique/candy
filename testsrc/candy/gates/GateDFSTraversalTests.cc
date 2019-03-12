/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
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

#include <candy/gates/GateDFSTraversal.h>
#include <candy/testutils/TestGateStructure.h>

#include <iostream>
#include <algorithm>

namespace Candy {
    struct TestGateCollector {
        std::vector<const Gate*> backtrackSequence {};
        std::vector<const Gate*> visitSequence {};
        size_t gateCount = 0;
        bool finishedCalled = false;
        
        void backtrack(const Gate* g) {
            backtrackSequence.push_back(g);
        }
        
        void collect(const Gate* g) {
            visitSequence.push_back(g);
        }
        
        void collectInput(Var var) {
            (void)var;
        }
        
        void init(size_t n) {
            gateCount = n;
        }
        
        bool pruneAt(const Gate& g) {
            return false;
        }
        
        void finished() {
            finishedCalled = true;
        }
    };
    
    static bool isNested(const GateAnalyzer& analyzer, const Gate& influenced, const Gate& influencer) {
        for (Lit l : influenced.getInputs()) {
            if (analyzer.getGate(l).isDefined()) {
                if (&analyzer.getGate(l) == &influencer
                    || isNested(analyzer, analyzer.getGate(l), influencer)) {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    /** checks if "g1 occurs before g2 => g1 is not nested in g2" for g1,g2 in gates. */
    static bool isConsistentBacktrackSequence(const GateAnalyzer& analyzer, const std::vector<const Gate*>& gates) {
        // this is a rather inefficient implementation, but this is just a test and the input data
        // is expected to be small, so this is reasonable
        for (size_t i = 0; i < gates.size()-1; ++i) {
            const Gate* former = gates[i];
            for (size_t j = i+1; j < gates.size()-1; ++j) {
                const Gate* latter = gates[j];
                if (isNested(analyzer, *former, *latter)) {
                    return false;
                }
            }
        }
        return true;
    }
    
    
    // For now, we only check if backtracking works correctly
    // TODO: validate visitation sequences also
    
    static void test_visitDFS(CNFProblem& problem, int nExpectedGates) {
        GateAnalyzer ga(problem);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), nExpectedGates);
        
        TestGateCollector collector = traverseDFS<TestGateCollector>(ga);
        
        EXPECT_TRUE(isConsistentBacktrackSequence(ga, collector.backtrackSequence));
        EXPECT_EQ(collector.gateCount, static_cast<unsigned long>(ga.getGateCount()));
        EXPECT_TRUE(collector.finishedCalled);
    }
    
    TEST(RSGateDFSTraversalTest, visitTinyGateStructure) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0,1));
        gateBuilder->withAnd({Lit(3, 1), Lit(4, 1)}, Lit(1,1));
        gateBuilder->withAnd({Lit(5, 1), Lit(4, 1)}, Lit(3,1));
        auto problem = gateBuilder->build();
        test_visitDFS(*problem, 3);
    }
    
    TEST(RSGateDFSTraversalTest, visitSmallGateStructure) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0,1));
        gateBuilder->withAnd({Lit(3, 1), Lit(4, 1)}, Lit(1,1));
        gateBuilder->withAnd({Lit(5, 1), Lit(4, 1), Lit(6, 1)}, Lit(3,0));
        gateBuilder->withAnd({Lit(7, 0), Lit(6, 0)}, Lit(4,1));
        gateBuilder->withOr({Lit(8, 1), Lit(9,0), Lit(5,0)}, Lit(7,0));
        gateBuilder->withAnd({Lit(10, 1), Lit(9,0), Lit(6,0)}, Lit(8,0));
        gateBuilder->withAnd({Lit(11, 1), Lit(12,0), Lit(13,0)}, Lit(5,0));
        
        auto problem = gateBuilder->build();
        test_visitDFS(*problem, 7);
    }
    
    struct NonmonotonicPruningTestGateCollector {
        std::vector<const Gate*> backtrackSequence {};
        std::vector<const Gate*> visitSequence {};
        size_t gateCount = 0;
        
        void backtrack(const Gate* g) {
            backtrackSequence.push_back(g);
        }
        
        void collect(const Gate* g) {
            visitSequence.push_back(g);
        }
        
        void collectInput(Var var) {
            (void)var;
        }
        
        void init(size_t n) {
            gateCount = n;
        }
        
        bool pruneAt(const Gate& g) {
            return g.hasNonMonotonousParent();
        }
        
        void finished() {
            
        }
    };
    
    namespace {
        bool containsGateWithOutput(std::vector<const Gate*>& gates, Var output) {
            return std::find_if(gates.begin(), gates.end(), [output](const Gate*& g) {
                return g->getOutput().var() == output;
            }) != gates.end();
        }
    }
    
    TEST(RSGateDFSTraversalTest, pruneNestedNonmonotonic) {
        auto gateBuilder = createGateStructureBuilder();
        
        // gates nested monotonic
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0,1));
        gateBuilder->withOr({Lit(3, 1), Lit(4, 1)}, Lit(1,1));
        gateBuilder->withOr({Lit(3, 1), Lit(5, 1)}, Lit(2,1));
        
        gateBuilder->withXor({Lit(6, 1), Lit(7,1)}, Lit(3,1));
        gateBuilder->withXor({Lit(7, 1), Lit(8,1)}, Lit(4,1));
        gateBuilder->withXor({Lit(9, 1), Lit(10,1)}, Lit(5,1));
        
        // gates nested nonmonotonic
        gateBuilder->withAnd({Lit(11, 1), Lit(12, 1)}, Lit(7,1));
        gateBuilder->withAnd({Lit(13, 1), Lit(14, 1)}, Lit(8,1));
        gateBuilder->withAnd({Lit(13, 1), Lit(14, 1)}, Lit(11,1));
        
        auto problem = gateBuilder->build();
        
        GateAnalyzer ga(*problem);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), 9);
        NonmonotonicPruningTestGateCollector collector = traverseDFS<NonmonotonicPruningTestGateCollector>(ga);
        EXPECT_TRUE(isConsistentBacktrackSequence(ga, collector.backtrackSequence));
        EXPECT_EQ(collector.visitSequence.size(), 6ull);
        EXPECT_TRUE(containsGateWithOutput(collector.visitSequence, 0));
        EXPECT_TRUE(containsGateWithOutput(collector.visitSequence, 1));
        EXPECT_TRUE(containsGateWithOutput(collector.visitSequence, 2));
        EXPECT_TRUE(containsGateWithOutput(collector.visitSequence, 3));
        EXPECT_TRUE(containsGateWithOutput(collector.visitSequence, 4));
        EXPECT_TRUE(containsGateWithOutput(collector.visitSequence, 5));
        EXPECT_FALSE(containsGateWithOutput(collector.visitSequence, 7));
        EXPECT_FALSE(containsGateWithOutput(collector.visitSequence, 8));
        EXPECT_FALSE(containsGateWithOutput(collector.visitSequence, 11));
    }
}
