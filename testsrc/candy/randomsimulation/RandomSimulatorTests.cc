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

#include <randomsimulation/RandomSimulator.h>
#include <gates/GateAnalyzer.h>

#include <testutils/TestGateStructure.h>
#include <testutils/TestUtils.h>
#include "TestUtils.h"

namespace Candy {
    
    TEST(RSRandomSimulatorTests, defaultImpl_noGates) {
        auto gateBuilder = createGateStructureBuilder();
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga(*formula);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), 0);
        
        auto underTest = createDefaultRandomSimulator(ga);
        auto result = underTest->run(2048);
        
        EXPECT_EQ(result.getBackbones().size(), 0ul);
        EXPECT_EQ(result.getEquivalences().size(), 0ul);
    }
    
    TEST(RSRandomSimulatorTests, defaultImpl_detectsSingleBackboneAndSingleEq) {
        auto gateBuilder = createGateStructureBuilder();
        
        gateBuilder->withOr({mkLit(1, 1), mkLit(2, 1)}, mkLit(0, 1));
        gateBuilder->withAnd({mkLit(3, 1), mkLit(4,1)}, mkLit(1,1));
        gateBuilder->withAnd({mkLit(3, 1), mkLit(4,1)}, mkLit(2,0));
        
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga(*formula);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), 3);
        
        auto underTest = createDefaultRandomSimulator(ga);
        
        auto result = underTest->run(4096);
        
        EXPECT_EQ(result.getBackbones().size(), 1ul);
        EXPECT_EQ(result.getEquivalences().size(), 1ul);
        EXPECT_TRUE(hasBackboneConj(result, mkLit(0,1)));
        EXPECT_TRUE(hasEquivalenceConj(result, {mkLit(1,1), mkLit(2,0)}));
    }
    
    
    
    static void test_defaultImpl_multipleBackbones(bool autoLen) {
        auto gateBuilder = createGateStructureBuilder();
        
        gateBuilder->withOr({mkLit(1, 1), mkLit(2, 1), mkLit(5,1)}, mkLit(0, 1));
        gateBuilder->withAnd({mkLit(3, 1), mkLit(4,1)}, mkLit(1,1));
        gateBuilder->withAnd({mkLit(3, 1), mkLit(4,1)}, mkLit(2,0));
        
        gateBuilder->withAnd({mkLit(6, 1), mkLit(7,1)}, mkLit(5,1));
        gateBuilder->withAnd({mkLit(8, 1), mkLit(9,1)}, mkLit(6,0));
        gateBuilder->withAnd({mkLit(8, 1), mkLit(9,1)}, mkLit(7,1));
        
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga(*formula);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), 6);
        
        auto rsBuilder = createDefaultRandomSimulatorBuilder();
        rsBuilder->withGateAnalyzer(ga);
        rsBuilder->withReductionRateAbortThreshold(0.1f);
        auto underTest = rsBuilder->build();
        
        Conjectures result;
        
        if (autoLen) {
            result = underTest->run();
        } else {
            result = underTest->run(4096);   
        }
        
        EXPECT_EQ(result.getBackbones().size(), 2ul);
        EXPECT_EQ(result.getEquivalences().size(), 2ul);
        EXPECT_TRUE(hasBackboneConj(result, mkLit(0,1)));
        EXPECT_TRUE(hasBackboneConj(result, mkLit(5,0)));
        EXPECT_TRUE(hasEquivalenceConj(result, {mkLit(1,1), mkLit(2,0)}));
        EXPECT_TRUE(hasEquivalenceConj(result, {mkLit(6,1), mkLit(7,0)}));
    }
    
    TEST(RSRandomSimulatorTests, defaultImpl_multipleBackbones_definedRunLength) {
        test_defaultImpl_multipleBackbones(false);
    }

    TEST(RSRandomSimulatorTests, defaultImpl_multipleBackbones_autoRunLength) {
        test_defaultImpl_multipleBackbones(true);
    }
}
