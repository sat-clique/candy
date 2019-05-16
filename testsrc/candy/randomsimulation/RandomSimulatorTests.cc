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

#include <candy/randomsimulation/RandomSimulator.h>
#include <candy/gates/GateAnalyzer.h>

#include <candy/testutils/TestGateStructure.h>
#include <candy/testutils/TestUtils.h>
#include "TestUtils.h"

namespace Candy {
    
    TEST(RSRandomSimulatorTests, defaultImpl_noGates) {
        auto gateBuilder = createGateStructureBuilder();
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga { *formula };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 0);

        BitparallelRandomSimulatorBuilder simulatorBuilder { ga };
        auto underTest = simulatorBuilder.build();

        auto result = underTest->run(2048);
        
        EXPECT_EQ(result.getBackbones().size(), 0ul);
        EXPECT_EQ(result.getEquivalences().size(), 0ul);
    }
    
    TEST(RSRandomSimulatorTests, defaultImpl_detectsSingleBackboneAndSingleEq) {
        auto gateBuilder = createGateStructureBuilder();
        
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0, 1));
        gateBuilder->withAnd({Lit(3, 1), Lit(4,1)}, Lit(1,1));
        gateBuilder->withAnd({Lit(3, 1), Lit(4,1)}, Lit(2,0));
        
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga { *formula };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 3);

        BitparallelRandomSimulatorBuilder simulatorBuilder { ga };
        auto underTest = simulatorBuilder.build();
        
        auto result = underTest->run(4096);
        
        EXPECT_EQ(result.getBackbones().size(), 1ul);
        EXPECT_EQ(result.getEquivalences().size(), 1ul);
        EXPECT_TRUE(hasBackboneConj(result, Lit(0,1)));
        EXPECT_TRUE(hasEquivalenceConj(result, {Lit(1,1), Lit(2,0)}));
    }
    
    
    
    static void test_defaultImpl_multipleBackbones(bool autoLen) {
        auto gateBuilder = createGateStructureBuilder();
        
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1), Lit(5,1)}, Lit(0, 1));
        gateBuilder->withAnd({Lit(3, 1), Lit(4,1)}, Lit(1,1));
        gateBuilder->withAnd({Lit(3, 1), Lit(4,1)}, Lit(2,0));
        
        gateBuilder->withAnd({Lit(6, 1), Lit(7,1)}, Lit(5,1));
        gateBuilder->withAnd({Lit(8, 1), Lit(9,1)}, Lit(6,0));
        gateBuilder->withAnd({Lit(8, 1), Lit(9,1)}, Lit(7,1));
        
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga { *formula };
        ga.analyze();
        ASSERT_EQ(ga.getResult().getGateCount(), 6);

        BitparallelRandomSimulatorBuilder simulatorBuilder { ga };
        auto underTest = simulatorBuilder.withReductionRateAbortThreshold(0.1f).build();
        
        Conjectures result;
        
        if (autoLen) {
            result = underTest->run();
        } else {
            result = underTest->run(4096);   
        }
        
        EXPECT_EQ(result.getBackbones().size(), 2ul);
        EXPECT_EQ(result.getEquivalences().size(), 2ul);
        EXPECT_TRUE(hasBackboneConj(result, Lit(0,1)));
        EXPECT_TRUE(hasBackboneConj(result, Lit(5,0)));
        EXPECT_TRUE(hasEquivalenceConj(result, {Lit(1,1), Lit(2,0)}));
        EXPECT_TRUE(hasEquivalenceConj(result, {Lit(6,1), Lit(7,0)}));
    }
    
    TEST(RSRandomSimulatorTests, defaultImpl_multipleBackbones_definedRunLength) {
        test_defaultImpl_multipleBackbones(false);
    }

    TEST(RSRandomSimulatorTests, defaultImpl_multipleBackbones_autoRunLength) {
        test_defaultImpl_multipleBackbones(true);
    }
}
