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
#include <core/SolverTypes.h>
#include <rsil/Utils.h>
#include <utils/FastRand.h>

#include <iostream>

namespace Candy {
    namespace {
        template<unsigned int tMaxAdviceSize>
        bool isEquivalenceAdvised(const ImplicitLearningAdvice<tMaxAdviceSize>& advice, Lit key, Lit equivalentLiteral) {
            auto& adviceEntry = advice.getAdvice(var(key));
            
            if (adviceEntry.m_isBackbone) {
                return false;
            }
            
            Lit searchedLit = sign(key) ? equivalentLiteral : ~equivalentLiteral;
            for (size_t i = 0; i < adviceEntry.m_size; ++i) {
                if (adviceEntry.m_lits[i] == searchedLit) {
                    return true;
                }
            }
            
            return false;
        }
        
        template<unsigned int tMaxAdviceSize>
        bool isBackboneAdvised(const ImplicitLearningAdvice<tMaxAdviceSize>& advice, Lit backboneLiteral) {
            auto& adviceEntry = advice.getAdvice(var(backboneLiteral));
            return adviceEntry.m_isBackbone
                    && adviceEntry.m_size == 1
                    && adviceEntry.m_lits[0] == backboneLiteral;
        }
    }
    
    
    TEST(RSILImplicitLearningAdviceTests, emptyConjectures) {
        Conjectures testData;
        ImplicitLearningAdvice<3> underTest(testData, 9);
        
        for (Var i = 0; i < 10; ++i) {
            EXPECT_FALSE(underTest.getAdvice(i).m_isBackbone);
            EXPECT_EQ(underTest.getAdvice(i).m_size, 0ull);
        }
    }
    
    TEST(RSILImplicitLearningAdviceTests, singleBackboneConjecture) {
        Conjectures testData;
        Lit backboneLit = mkLit(5,0);
        testData.addBackbone(BackboneConjecture{backboneLit});
        ImplicitLearningAdvice<3> underTest(testData, 9);
        
        for (Var i = 0; i < 10; ++i) {
            if (i != var(backboneLit)) {
                EXPECT_FALSE(underTest.getAdvice(i).m_isBackbone);
                EXPECT_EQ(underTest.getAdvice(i).m_size, 0ull);
            }
        }
        
        EXPECT_TRUE(underTest.hasPotentialAdvice(var(backboneLit)));
        EXPECT_TRUE(isBackboneAdvised(underTest, backboneLit));
    }
    
    TEST(RSILImplicitLearningAdviceTests, singleEquivalenceConjecture) {
        Conjectures testData;
 
        testData.addEquivalence(EquivalenceConjecture{std::vector<Lit>{mkLit(5,1), mkLit(1,0)}});
        ImplicitLearningAdvice<3> underTest(testData, 5);
        
        EXPECT_TRUE(underTest.hasPotentialAdvice(5));
        EXPECT_TRUE(isEquivalenceAdvised(underTest, mkLit(5,1), mkLit(1,0)));
        EXPECT_TRUE(isEquivalenceAdvised(underTest, mkLit(1,1), mkLit(5,0)));
        EXPECT_FALSE(isEquivalenceAdvised(underTest, mkLit(5,1), mkLit(1,1)));
        EXPECT_FALSE(isEquivalenceAdvised(underTest, mkLit(1,1), mkLit(5,1)));
        EXPECT_FALSE(isEquivalenceAdvised(underTest, mkLit(1,1), mkLit(4,1)));
    }
    
    TEST(RSILImplicitLearningAdviceTests, oversizedEquivalenceConjectureIsntAdded) {
        Conjectures testData;
        
        testData.addEquivalence(EquivalenceConjecture{std::vector<Lit>{mkLit(5,1), mkLit(1,0), mkLit(2,1)}});
        ImplicitLearningAdvice<2> underTest(testData, 5);
        
        EXPECT_FALSE(isEquivalenceAdvised(underTest, mkLit(5,1), mkLit(1,0)));
        EXPECT_FALSE(isEquivalenceAdvised(underTest, mkLit(1,1), mkLit(5,0)));
    }
}
