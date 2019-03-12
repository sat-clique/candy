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
#include <candy/randomsimulation/Conjectures.h>

#include "TestUtils.h"

namespace Candy {
    TEST(RSConjectureFilterTest, filterBySize2) {
        Conjectures testData;
        
        std::vector<Lit> remainingEq{Lit(0, 1), Lit(1, 1)};
        Lit remainingBB = Lit(5,1);
        
        testData.addEquivalence(EquivalenceConjecture{remainingEq});
        testData.addEquivalence(EquivalenceConjecture{{Lit(2, 1), Lit(3, 1), Lit(4, 1)}});
        testData.addBackbone(BackboneConjecture{remainingBB});
        
        auto filterUnderTest = createSizeConjectureFilter(2ull);
        auto result = filterUnderTest->apply(testData);
        
        EXPECT_EQ(result.getEquivalences().size(), 1ull);
        EXPECT_EQ(result.getBackbones().size(), 1ull);
        EXPECT_TRUE(hasBackboneConj(result, remainingBB));
        EXPECT_TRUE(hasEquivalenceConj(result, remainingEq));
    }
    
    TEST(RSConjectureFilterTest, filterBySize2ProducesEmptyOutputForEmptyInput) {
        Conjectures testData;
        
        auto filterUnderTest = createSizeConjectureFilter(2ull);
        auto result = filterUnderTest->apply(testData);
        
        EXPECT_EQ(result.getEquivalences().size(), 0ull);
        EXPECT_EQ(result.getBackbones().size(), 0ull);
    }
    
    TEST(RSConjectureFilterTest, filterBySize1RemovesAllEquivalenceConjectures) {
        Conjectures testData;
        
        std::vector<Lit> remainingEq{Lit(0, 1), Lit(1, 1)};
        Lit remainingBB = Lit(5,1);
        
        testData.addEquivalence(EquivalenceConjecture{remainingEq});
        testData.addEquivalence(EquivalenceConjecture{{Lit(2, 1), Lit(3, 1), Lit(4, 1)}});
        testData.addBackbone(BackboneConjecture{remainingBB});
        
        auto filterUnderTest = createSizeConjectureFilter(1ull);
        auto result = filterUnderTest->apply(testData);
        
        EXPECT_EQ(result.getEquivalences().size(), 0ull);
    }
    
    TEST(RSConjectureFilterTest, backboneRemovalFilterRemovesAllBackbones) {
        Conjectures testData;
        
        std::vector<Lit> remainingEq1{Lit(0, 1), Lit(1, 1)};
        std::vector<Lit> remainingEq2{Lit(2, 1), Lit(3, 1)};
        
        testData.addEquivalence(EquivalenceConjecture{remainingEq1});
        testData.addEquivalence(EquivalenceConjecture{remainingEq2});
        testData.addBackbone(BackboneConjecture{Lit(4,1)});
        testData.addBackbone(BackboneConjecture{Lit(5,1)});
        
        auto filterUnderTest = createBackboneRemovalConjectureFilter();
        auto result = filterUnderTest->apply(testData);
        
        EXPECT_EQ(result.getEquivalences().size(), 2ull);
        EXPECT_EQ(result.getBackbones().size(), 0ull);
        EXPECT_TRUE(hasEquivalenceConj(result, remainingEq1));
        EXPECT_TRUE(hasEquivalenceConj(result, remainingEq2));
    }
    
    TEST(RSConjectureFilterTest, backboneRemovalFilterProducesEmptyOutputForEmptyInput) {
        Conjectures testData;
        
        auto filterUnderTest = createBackboneRemovalConjectureFilter();
        auto result = filterUnderTest->apply(testData);
        
        EXPECT_EQ(result.getEquivalences().size(), 0ull);
        EXPECT_EQ(result.getBackbones().size(), 0ull);
    }
}
