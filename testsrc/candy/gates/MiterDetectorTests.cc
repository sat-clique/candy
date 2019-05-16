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
    TEST(GateMiterDetectorTests, rejectsMonotonousNonmiterStructure) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withAnd({Lit(1, 1), Lit(2, 1)}, Lit(0, 1));
        gateBuilder->withAnd({Lit(3, 1), Lit(4, 1)}, Lit(1, 1));
        gateBuilder->withAnd({Lit(3, 1), Lit(5, 1)}, Lit(2, 1));
        gateBuilder->withXor({Lit(6, 1), Lit(7, 1)}, Lit(3, 1));
        gateBuilder->withXor({Lit(7, 1), Lit(8, 1)}, Lit(4, 1));
        gateBuilder->withXor({Lit(9, 1), Lit(10, 1)}, Lit(5, 1));

        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer {*problem};
        analyzer.analyze();
        ASSERT_EQ(analyzer.getResult().getGateCount(), 6);
        EXPECT_FALSE(hasPossiblyMiterStructure(analyzer));
    }
    
    TEST(GateMiterDetectorTests, rejectsMonotonousMiterStructure) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withAnd({Lit(1, 1), Lit(2, 1)}, Lit(0, 1));
        gateBuilder->withAnd({Lit(3, 1), Lit(4, 1)}, Lit(1, 1));
        gateBuilder->withAnd({Lit(3, 1), Lit(5, 1)}, Lit(2, 1));
        gateBuilder->withXor({Lit(6, 1), Lit(7, 1)}, Lit(3, 1));
        gateBuilder->withXor({Lit(8, 1), Lit(9, 1)}, Lit(4, 1));
        gateBuilder->withXor({Lit(10, 1), Lit(11, 1)}, Lit(5, 1));
        
        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer {*problem};
        analyzer.analyze();
        ASSERT_EQ(analyzer.getResult().getGateCount(), 6);
        EXPECT_FALSE(hasPossiblyMiterStructure(analyzer));
    }
    
    TEST(GateMiterDetectorTests, rejectsEmptyStructure) {
        auto gateBuilder = createGateStructureBuilder();
        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer {*problem};
        analyzer.analyze();
        ASSERT_EQ(analyzer.getResult().getGateCount(), 0);
        EXPECT_FALSE(hasPossiblyMiterStructure(analyzer));
    }
    
    TEST(GateMiterDetectorTests, rejectsNearMiter_nonAllXOR) {
        auto gateBuilder = createGateStructureBuilder();
        // miter "base" structure
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0, 1));
        gateBuilder->withOr({Lit(3, 1), Lit(4, 1)}, Lit(1, 1));
        gateBuilder->withOr({Lit(5, 1), Lit(6, 1)}, Lit(2, 1));
        gateBuilder->withXor({Lit(7, 1), Lit(8, 1)}, Lit(3, 1));
        gateBuilder->withXor({Lit(9, 1), Lit(10, 1)}, Lit(4, 1));
        gateBuilder->withXor({Lit(11, 1), Lit(12,1)}, Lit(5, 1));
        gateBuilder->withAnd({Lit(13, 1), Lit(14,1)}, Lit(6, 1)); // "bad" gate
        
        // circuit 1
        gateBuilder->withAnd({Lit(15, 1), Lit(16, 1)}, Lit(7, 1));
        gateBuilder->withAnd({Lit(15, 1), Lit(16, 1)}, Lit(8, 1));
        
        // circuit 2
        gateBuilder->withOr({Lit(17, 1), Lit(18, 1)}, Lit(9, 0));
        gateBuilder->withOr({Lit(17, 1), Lit(18, 1)}, Lit(10, 0));
        
        // circuit 3
        gateBuilder->withXor({Lit(17, 1), Lit(18, 1)}, Lit(11, 0));
        gateBuilder->withXor({Lit(17, 1), Lit(18, 1)}, Lit(12, 0));
        
        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer {*problem};
        analyzer.analyze();
        ASSERT_EQ(analyzer.getResult().getGateCount(), 13);
        EXPECT_FALSE(hasPossiblyMiterStructure(analyzer));
    }
    
    TEST(GateMiterDetectorTests, rejectsNearMiter_sharedInputs) {
        auto gateBuilder = createGateStructureBuilder();
        // miter "base" structure
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0, 1));
        gateBuilder->withOr({Lit(3, 1), Lit(4, 1)}, Lit(1, 1));
        gateBuilder->withOr({Lit(5, 1), Lit(6, 1)}, Lit(2, 1));
        gateBuilder->withXor({Lit(7, 1), Lit(8, 1)}, Lit(3, 1));
        gateBuilder->withXor({Lit(9, 1), Lit(10, 1)}, Lit(4, 1));
        gateBuilder->withXor({Lit(11, 1), Lit(12,1)}, Lit(5, 1));
        gateBuilder->withXor({Lit(12, 1), Lit(14,1)}, Lit(6, 1)); // "bad" gate
        
        // circuit 1
        gateBuilder->withAnd({Lit(15, 1), Lit(16, 1)}, Lit(7, 1));
        gateBuilder->withAnd({Lit(15, 1), Lit(16, 1)}, Lit(8, 1));
        
        // circuit 2
        gateBuilder->withOr({Lit(17, 1), Lit(18, 1)}, Lit(9, 0));
        gateBuilder->withOr({Lit(17, 1), Lit(18, 1)}, Lit(10, 0));
        
        // circuit 3
        gateBuilder->withXor({Lit(17, 1), Lit(18, 1)}, Lit(11, 0));
        gateBuilder->withXor({Lit(17, 1), Lit(18, 1)}, Lit(12, 0));
        
        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer {*problem};
        analyzer.analyze();
        ASSERT_EQ(analyzer.getResult().getGateCount(), 13);
        EXPECT_FALSE(hasPossiblyMiterStructure(analyzer));
    }
    
    TEST(GateMiterDetectorTests, acceptsNonAIGMiter) {
        auto gateBuilder = createGateStructureBuilder();
        // miter "base" structure
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0, 1));
        gateBuilder->withOr({Lit(3, 1), Lit(4, 1)}, Lit(1, 1));
        gateBuilder->withOr({Lit(5, 1), Lit(6, 1)}, Lit(2, 1));
        gateBuilder->withXor({Lit(7, 1), Lit(8, 1)}, Lit(3, 1));
        gateBuilder->withXor({Lit(9, 1), Lit(10, 1)}, Lit(4, 1));
        gateBuilder->withXor({Lit(11, 1), Lit(12,1)}, Lit(5, 1));
        gateBuilder->withXor({Lit(13, 1), Lit(14,1)}, Lit(6, 1));
        
        // circuit 1
        gateBuilder->withAnd({Lit(15, 1), Lit(16, 1)}, Lit(7, 1));
        gateBuilder->withAnd({Lit(15, 1), Lit(16, 1)}, Lit(8, 1));
        
        // circuit 2
        gateBuilder->withOr({Lit(17, 1), Lit(18, 1)}, Lit(9, 0));
        gateBuilder->withOr({Lit(17, 1), Lit(18, 1)}, Lit(10, 0));
        
        // circuit 3
        gateBuilder->withXor({Lit(17, 1), Lit(18, 1)}, Lit(11, 0));
        gateBuilder->withXor({Lit(17, 1), Lit(18, 1)}, Lit(12, 0));
        
        // circuit 4 (var 14 remains input, "single-line circuit" case)
        gateBuilder->withXor({Lit(17, 1), Lit(18, 1)}, Lit(13, 0));
        
        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer {*problem};
        analyzer.analyze();
        ASSERT_EQ(analyzer.getResult().getGateCount(), 14);
        EXPECT_TRUE(hasPossiblyMiterStructure(analyzer));
    }
    
    namespace {
        void addAIGXOR(GateStructureBuilder& gateBuilder,
                       Lit input1, Lit input2,
                       Lit intermediate1, Lit intermediate2,
                       Lit output) {
            gateBuilder.withAnd({~intermediate1, ~intermediate2}, output);
            gateBuilder.withAnd({input1, ~input2}, intermediate1);
            gateBuilder.withAnd({~input1, input2}, intermediate2);
        }
    }
    
    TEST(GateMiterDetectorTests, acceptsAIGMiter) {
        auto gateBuilder = createGateStructureBuilder();
        // miter "base" structure
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0, 1));
        gateBuilder->withOr({Lit(3, 1), Lit(4, 1)}, Lit(1, 1));
        
        addAIGXOR(*gateBuilder,
                  Lit(7, 1), Lit(8, 1),
                  Lit(9, 1), Lit(10, 1),
                  Lit(2, 1));
        
        addAIGXOR(*gateBuilder,
                  Lit(11, 1), Lit(12, 1),
                  Lit(13, 1), Lit(14, 1),
                  Lit(3, 1));
        
        addAIGXOR(*gateBuilder,
                  Lit(15, 1), Lit(16, 1),
                  Lit(17, 1), Lit(18, 1),
                  Lit(4, 1));
        
        // add at least one nonmonotonously nested gate
        gateBuilder->withAnd({Lit(19, 1), Lit(20, 1)}, Lit(15, 1));
        gateBuilder->withAnd({Lit(19, 1), Lit(20, 1)}, Lit(16, 1));
        
        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer {*problem};
        analyzer.analyze();
        ASSERT_EQ(analyzer.getResult().getGateCount(), 13);
        EXPECT_TRUE(hasPossiblyMiterStructure(analyzer));
    }
    
    TEST(GateMiterDetectorTests, rejectsNearAIGMiter) {
        auto gateBuilder = createGateStructureBuilder();
        // miter "base" structure
        gateBuilder->withOr({Lit(1, 1), Lit(2, 1)}, Lit(0, 1));
        gateBuilder->withOr({Lit(3, 1), Lit(4, 1)}, Lit(1, 1));
        
        addAIGXOR(*gateBuilder,
                  Lit(7, 1), Lit(8, 1),
                  Lit(9, 1), Lit(10, 1),
                  Lit(2, 1));
        
        addAIGXOR(*gateBuilder,
                  Lit(11, 1), Lit(12, 1),
                  Lit(13, 1), Lit(14, 1),
                  Lit(3, 1));
        
        gateBuilder->withAnd({Lit(17, 1), Lit(18, 1)}, Lit(4, 1)); // "bad" gate
        
        // add at least one nonmonotonously nested gate
        gateBuilder->withAnd({Lit(19, 1), Lit(20, 1)}, Lit(11, 1));
        gateBuilder->withAnd({Lit(19, 1), Lit(20, 1)}, Lit(12, 1));
        
        auto problem = gateBuilder->build();
        
        GateAnalyzer analyzer {*problem};
        analyzer.analyze();
        ASSERT_EQ(analyzer.getResult().getGateCount(), 11);
        EXPECT_FALSE(hasPossiblyMiterStructure(analyzer));
    }
    
    namespace {
        void miterdetectionAcceptanceTest(const std::string& filename, bool expectedResult) {
            std::string fullFilename = "problems/" + filename + ".cnf";
            
            CNFProblem problem;
            problem.readDimacsFromFile(fullFilename.c_str());
            ASSERT_FALSE(problem.nClauses() == 0) << "Could not read test problem file.";
            
            GateAnalyzer analyzer{problem};
            analyzer.analyze();
            EXPECT_EQ(hasPossiblyMiterStructure(analyzer), expectedResult);
        }
    }
    
    TEST(GateMiterDetectorTests, acceptance_acceptsAIGMiter_6s33) {
        miterdetectionAcceptanceTest("6s33", true);
    }
    
    TEST(GateMiterDetectorTests, acceptance_rejectsNonMiter_flat200_1) {
        miterdetectionAcceptanceTest("flat200-1", false);
    }
}
