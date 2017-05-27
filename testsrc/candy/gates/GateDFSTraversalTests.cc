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

#include <testutils/TestGateStructure.h>

#include <iostream>

namespace Candy {
    struct TestGateCollector {
        std::vector<Gate*> backtrackSequence {};
        std::vector<Gate*> visitSequence {};
        size_t gateCount = 0;
        
        void backtrack(Gate* g) {
            backtrackSequence.push_back(g);
        }
        
        void collect(Gate* g) {
            visitSequence.push_back(g);
        }
        
        void collectInput(Var var) {
            (void)var;
        }
        
        void init(size_t n) {
            gateCount = n;
        }
    };
    
    static bool isNested(GateAnalyzer& analyzer, Gate& influenced, Gate& influencer) {
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
    static bool isConsistentBacktrackSequence(GateAnalyzer& analyzer, const std::vector<Gate*>& gates) {
        // this is a rather inefficient implementation, but this is just a test and the input data
        // is expected to be small, so this is reasonable
        for (size_t i = 0; i < gates.size()-1; ++i) {
            Gate* former = gates[i];
            for (size_t j = i+1; j < gates.size()-1; ++j) {
                Gate* latter = gates[j];
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
    }
    
    TEST(RSGateDFSTraversalTest, visitTinyGateStructure) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({mkLit(1, 1), mkLit(2, 1)}, mkLit(0,1));
        gateBuilder->withAnd({mkLit(3, 1), mkLit(4, 1)}, mkLit(1,1));
        gateBuilder->withAnd({mkLit(5, 1), mkLit(4, 1)}, mkLit(3,1));
        auto problem = gateBuilder->build();
        test_visitDFS(*problem, 3);
    }
    
    TEST(RSGateDFSTraversalTest, visitSmallGateStructure) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({mkLit(1, 1), mkLit(2, 1)}, mkLit(0,1));
        gateBuilder->withAnd({mkLit(3, 1), mkLit(4, 1)}, mkLit(1,1));
        gateBuilder->withAnd({mkLit(5, 1), mkLit(4, 1), mkLit(6, 1)}, mkLit(3,0));
        gateBuilder->withAnd({mkLit(7, 0), mkLit(6, 0)}, mkLit(4,1));
        gateBuilder->withOr({mkLit(8, 1), mkLit(9,0), mkLit(5,0)}, mkLit(7,0));
        gateBuilder->withAnd({mkLit(10, 1), mkLit(9,0), mkLit(6,0)}, mkLit(8,0));
        gateBuilder->withAnd({mkLit(11, 1), mkLit(12,0), mkLit(13,0)}, mkLit(5,0));
        
        auto problem = gateBuilder->build();
        test_visitDFS(*problem, 7);
    }
}
