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
#include <randomsimulation/ClauseOrder.h>

#include <gates/GateAnalyzer.h>
#include <candy/testutils/TestGateStructure.h>

namespace Candy {
    
    TEST(RSGateFiltersTest, nonMonoFilterEmptyGateStructureProducesEmptyResult) {
        auto testDataBuilder = createGateStructureBuilder();
        auto testData = testDataBuilder->build();
        GateAnalyzer ga{*testData};
        ga.analyze();
        
        auto underTest = createNonmonotonousGateFilter(ga);
        
        EXPECT_EQ(underTest->getEnabledOutputVars().size(), 0ull);
    }
    
    TEST(RSGateFiltersTest, nonMonoFilterOnlyMonoNestedGatesProducesEmptyResult) {
        auto testDataBuilder = createGateStructureBuilder();
        testDataBuilder->withAnd({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0, 1));
        testDataBuilder->withAnd({Glucose::mkLit(3, 1), Glucose::mkLit(4, 1)}, Glucose::mkLit(1, 1));
        testDataBuilder->withAnd({Glucose::mkLit(5, 1), Glucose::mkLit(6, 1)}, Glucose::mkLit(2, 1));
        
        auto testData = testDataBuilder->build();
        GateAnalyzer ga{*testData};
        ga.analyze();
        
        ASSERT_EQ(ga.getGateCount(), 3);
        
        auto underTest = createNonmonotonousGateFilter(ga);
        
        EXPECT_EQ(underTest->getEnabledOutputVars().size(), 0ull);
    }
    
    TEST(RSGateFiltersTest, nonMonoFilterRetainsNonMonoNestedGates) {
        auto testDataBuilder = createGateStructureBuilder();
        
        // this gate is expected to be removed
        testDataBuilder->withAnd({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0, 1));
        
        // this gate is expected to be removed
        testDataBuilder->withXor({Glucose::mkLit(3, 0), Glucose::mkLit(4, 1)}, Glucose::mkLit(1, 1));

        // this gate is expected to remain
        testDataBuilder->withAnd({Glucose::mkLit(5, 1), Glucose::mkLit(6, 1)}, Glucose::mkLit(3, 1));
        
        // this gate is expected to remain
        testDataBuilder->withAnd({Glucose::mkLit(7, 1), Glucose::mkLit(8, 1)}, Glucose::mkLit(4, 1));
        
        auto testData = testDataBuilder->build();
        GateAnalyzer ga{*testData};
        ga.analyze();
        
        ASSERT_EQ(ga.getGateCount(), 4);
        
        auto underTest = createNonmonotonousGateFilter(ga);
        
        auto enabledVars = underTest->getEnabledOutputVars();
        EXPECT_EQ(enabledVars.size(), 2ull);
        EXPECT_TRUE(enabledVars.find(3) != enabledVars.end());
        EXPECT_TRUE(enabledVars.find(4) != enabledVars.end());
    }
}
