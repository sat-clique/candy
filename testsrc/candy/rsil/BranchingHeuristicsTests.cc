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
#include <rsil/BranchingHeuristics.h>

namespace Candy {
    TEST(RSILBranchingHeuristicsTests, uninitializedHeuristicReturnsUndefForMinInput) {
        RSILBranchingHeuristic<3> underTest;
        auto result = underTest.getAdvice({mkLit(0)}, {0}, {}, {0});
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, emptyInitializedHeuristicReturnsUndefForMinInput) {
        RSILBranchingHeuristic<3>::Parameters params{{}};
        RSILBranchingHeuristic<3> underTest{params};
        
        auto result = underTest.getAdvice({mkLit(0)}, {0}, {}, {0});
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBranchingHeuristic<3>::Parameters params{testData};
        RSILBranchingHeuristic<3> underTest{params};
        
        RSILBranchingHeuristic<3>::TrailType testTrail {mkLit(1,0)};
        RSILBranchingHeuristic<3>::TrailLimType testTrailLim {0};
        RSILBranchingHeuristic<3>::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        RSILBranchingHeuristic<3>::AssignsType testAssigns {l_True, l_False, l_Undef, l_Undef, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        ASSERT_NE(result, lit_Undef);
        EXPECT_EQ(result, mkLit(2, 0));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBranchingHeuristic<3>::Parameters params{testData};
        RSILBranchingHeuristic<3> underTest{params};
        
        RSILBranchingHeuristic<3>::TrailType testTrail {mkLit(1,0)};
        RSILBranchingHeuristic<3>::TrailLimType testTrailLim {0};
        RSILBranchingHeuristic<3>::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        RSILBranchingHeuristic<3>::AssignsType testAssigns {l_True, l_False, l_True, l_Undef, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBranchingHeuristic<3>::Parameters params{testData};
        RSILBranchingHeuristic<3> underTest{params};
        
        RSILBranchingHeuristic<3>::TrailType testTrail {mkLit(1,0)};
        RSILBranchingHeuristic<3>::TrailLimType testTrailLim {0};
        RSILBranchingHeuristic<3>::DecisionType testDecisionVars {1, 1, 0, 1, 1};
        RSILBranchingHeuristic<3>::AssignsType testAssigns {l_True, l_False, l_True, l_Undef, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBranchingHeuristic<3>::Parameters params{testData};
        RSILBranchingHeuristic<3> underTest{params};
        
        RSILBranchingHeuristic<3>::TrailType testTrail {mkLit(4,0)};
        RSILBranchingHeuristic<3>::TrailLimType testTrailLim {0};
        RSILBranchingHeuristic<3>::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        RSILBranchingHeuristic<3>::AssignsType testAssigns {l_True, l_True, l_Undef, l_Undef, l_False};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        RSILBranchingHeuristic<3>::Parameters params{testData};
        RSILBranchingHeuristic<3> underTest{params};
        
        RSILBranchingHeuristic<3>::TrailType testTrail {mkLit(3,0)};
        RSILBranchingHeuristic<3>::TrailLimType testTrailLim {0};
        RSILBranchingHeuristic<3>::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        RSILBranchingHeuristic<3>::AssignsType testAssigns {l_Undef, l_Undef, l_Undef, l_False, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        ASSERT_NE(result, lit_Undef);
        
        EXPECT_TRUE((result == mkLit(1, 0)) || (result == mkLit(2,0)));
    }
    
    TEST(RSILBranchingHeuristicsTests, travelsUpTrail) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        RSILBranchingHeuristic<3>::Parameters params{testData};
        RSILBranchingHeuristic<3> underTest{params};
        
        RSILBranchingHeuristic<3>::TrailType testTrail {mkLit(3,0), mkLit(4,0)};
        RSILBranchingHeuristic<3>::TrailLimType testTrailLim {0};
        RSILBranchingHeuristic<3>::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        RSILBranchingHeuristic<3>::AssignsType testAssigns {l_Undef, l_Undef, l_Undef, l_False, l_False};
        
        auto result = underTest.getAdvice(testTrail, testTrailLim, testAssigns, testDecisionVars);
        ASSERT_NE(result, lit_Undef);
        
        EXPECT_TRUE((result == mkLit(1, 0)) || (result == mkLit(2,0)));
    }
}
