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

#include <candy/gates/GateAnalyzer.h>
#include <candy/gates/MiterDetector.h>
#include <candy/testutils/TestGateStructure.h>
#include <candy/core/SolverTypes.h>

namespace Candy {
    TEST(GateMiterDetectorTests, rejectsMonotonousStructure) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withAnd({mkLit(1, 1), mkLit(2, 1)}, mkLit(0,1));
        gateBuilder->withAnd({mkLit(3, 1), mkLit(4, 1)}, mkLit(1,1));
        gateBuilder->withAnd({mkLit(3, 1), mkLit(5, 1)}, mkLit(2,1));
        gateBuilder->withXor({mkLit(6, 1), mkLit(7,1)}, mkLit(3,1));
        gateBuilder->withXor({mkLit(7, 1), mkLit(8,1)}, mkLit(4,1));
        gateBuilder->withXor({mkLit(9, 1), mkLit(10,1)}, mkLit(5,1));

        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer(*problem);
        analyzer.analyze();
        ASSERT_EQ(analyzer.getGateCount(), 6);
        EXPECT_FALSE(hasPossiblyMiterStructure(analyzer));
    }
    
    TEST(GateMiterDetectorTests, rejectsEmptyStructure) {
        auto gateBuilder = createGateStructureBuilder();
        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer(*problem);
        analyzer.analyze();
        ASSERT_EQ(analyzer.getGateCount(), 0);
        EXPECT_FALSE(hasPossiblyMiterStructure(analyzer));
    }
    
    TEST(GateMiterDetectorTests, rejectsNearMiter) {
        auto gateBuilder = createGateStructureBuilder();
        // miter "base" structure
        gateBuilder->withOr({mkLit(1, 1), mkLit(2, 1)}, mkLit(0, 1));
        gateBuilder->withOr({mkLit(3, 1), mkLit(4, 1)}, mkLit(1, 1));
        gateBuilder->withOr({mkLit(5, 1), mkLit(6, 1)}, mkLit(2, 1));
        gateBuilder->withXor({mkLit(7, 1), mkLit(8, 1)}, mkLit(3, 1));
        gateBuilder->withXor({mkLit(9, 1), mkLit(10, 1)}, mkLit(4, 1));
        gateBuilder->withXor({mkLit(11, 1), mkLit(12,1)}, mkLit(5, 1));
        gateBuilder->withAnd({mkLit(13, 1), mkLit(14,1)}, mkLit(6, 1));
        
        // circuit 1
        gateBuilder->withAnd({mkLit(15, 1), mkLit(16, 1)}, mkLit(7, 1));
        gateBuilder->withAnd({mkLit(15, 1), mkLit(16, 1)}, mkLit(8, 1));
        
        // circuit 2
        gateBuilder->withOr({mkLit(17, 1), mkLit(18, 1)}, mkLit(9, 0));
        gateBuilder->withOr({mkLit(17, 1), mkLit(18, 1)}, mkLit(10, 0));
        
        // circuit 3
        gateBuilder->withXor({mkLit(17, 1), mkLit(18, 1)}, mkLit(11, 0));
        gateBuilder->withXor({mkLit(17, 1), mkLit(18, 1)}, mkLit(12, 0));
        
        // circuit 4 (var 14 remains input, "single-line circuit" case)
        gateBuilder->withXor({mkLit(17, 1), mkLit(18, 1)}, mkLit(13, 0));
        
        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer(*problem);
        analyzer.analyze();
        ASSERT_EQ(analyzer.getGateCount(), 14);
        EXPECT_FALSE(hasPossiblyMiterStructure(analyzer));
    }
}
