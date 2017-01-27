/* Copyright (c) 2017 Felix Kutzner
 
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
#include <gmock/gmock.h>

#include "HeuristicsMock.h"
#include "TestUtils.h"

#include <rsar/Refinement.h>
#include <rsar/ApproximationState.h>
#include <rsar/Heuristics.h>
#include <randomsimulation/Conjectures.h>

#include <functional>

namespace Candy {
    static bool contains(Cl& clause, Lit lit) {
        return std::find(clause.begin(), clause.end(), lit) != clause.end();
    }
    
    TEST(RSARClauseEncoding, encodeBackbone) {
        BackboneConjecture backbone {Glucose::mkLit(10,1)};
        Var assumptionVar = 100;
        auto clause = encodeBackbone(backbone, assumptionVar);
        
        EXPECT_EQ(clause.size(), 2ull);
        EXPECT_TRUE(contains(clause, Glucose::mkLit(10,1)));
        EXPECT_TRUE(contains(clause, deactivatedAssumptionLit(assumptionVar)));
    }
    
    TEST(RSARClauseEncoding, encodeImplication) {
        Implication impl {Glucose::mkLit(10, 0), Glucose::mkLit(1, 0)};
        
        Var assumptionVar = 100;
        auto clause = encodeImplication(impl, assumptionVar);
        
        EXPECT_EQ(clause.size(), 3ull);
        EXPECT_TRUE(contains(clause, Glucose::mkLit(10,1)));
        EXPECT_TRUE(contains(clause, Glucose::mkLit(1,0)));
        EXPECT_TRUE(contains(clause, deactivatedAssumptionLit(assumptionVar)));
    }
    
    TEST(RSARClauseEncoding, assumptionActivation) {
        EXPECT_TRUE(isActive(activatedAssumptionLit(10)));
        EXPECT_EQ(activatedAssumptionLit(10), ~deactivatedAssumptionLit(10));
    }
    
    TEST(RSARRefinementStrategy, refineEmptyNoHeuristics) {
        Candy::Conjectures testData;
        std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> heuristics;
        heuristics.reset(new std::vector<std::unique_ptr<RefinementHeuristic>>{});
        std::function<Var()> newVarFn;
        
        auto underTest = createDefaultRefinementStrategy(testData,
                                                         std::move(heuristics),
                                                         newVarFn);
        
        auto initDelta = underTest->init();
        EXPECT_TRUE(initDelta->getNewClauses().empty());
        EXPECT_TRUE(initDelta->getAssumptionLiterals().empty());
        EXPECT_EQ(initDelta->countEnabledClauses(), 0ull);
        
        auto sndDelta = underTest->refine();
        EXPECT_TRUE(sndDelta->getNewClauses().empty());
        EXPECT_TRUE(sndDelta->getAssumptionLiterals().empty());
        EXPECT_EQ(sndDelta->countEnabledClauses(), 0ull);
    }
    
    TEST(RSARRefinementStrategy, refineEmptyWithNoopHeuristic) {
        auto mock = std::unique_ptr<MockHeuristic>(new MockHeuristic());
        EXPECT_CALL(*mock, beginRefinementStep()).Times(testing::Exactly(2));
        EXPECT_CALL(*mock, markRemovals(testing::Matcher<EquivalenceImplications&>(testing::_))).Times(testing::Exactly(0));
        EXPECT_CALL(*mock, markRemovals(testing::Matcher<Backbones&>(testing::_))).Times(testing::Exactly(0));
        
        Candy::Conjectures testData;
        std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> heuristics;
        heuristics.reset(new std::vector<std::unique_ptr<RefinementHeuristic>>{});
        heuristics->push_back(std::move(mock));
        
        std::function<Var()> newVarFn;
        
        auto underTest = createDefaultRefinementStrategy(testData,
                                                         std::move(heuristics),
                                                         newVarFn);
        
        auto initDelta = underTest->init();
        EXPECT_TRUE(initDelta->getNewClauses().empty());
        EXPECT_TRUE(initDelta->getAssumptionLiterals().empty());
        EXPECT_EQ(initDelta->countEnabledClauses(), 0ull);
        
        auto sndDelta = underTest->refine();
        EXPECT_TRUE(sndDelta->getNewClauses().empty());
        EXPECT_TRUE(sndDelta->getAssumptionLiterals().empty());
        EXPECT_EQ(sndDelta->countEnabledClauses(), 0ull);
    }
    

    
    TEST(RSARRefinementStrategy, noopRefineWithTwoEquivalenceClasses) {
        auto mock = std::unique_ptr<MockHeuristic>(new MockHeuristic());
        
        EXPECT_CALL(*mock, beginRefinementStep()).Times(testing::Exactly(2));
        EXPECT_CALL(*mock, markRemovals(testing::Matcher<EquivalenceImplications&>(testing::_))).Times(testing::Exactly(4));
        EXPECT_CALL(*mock, markRemovals(testing::Matcher<Backbones&>(testing::_))).Times(testing::Exactly(2));
        
        Candy::Conjectures testData;
        
        EquivalenceConjecture eqc1;
        eqc1.addLit(Glucose::mkLit(0, 1));
        eqc1.addLit(Glucose::mkLit(1, 0));
        eqc1.addLit(Glucose::mkLit(2, 0));
        
        EquivalenceConjecture eqc2;
        eqc2.addLit(Glucose::mkLit(3, 1));
        eqc2.addLit(Glucose::mkLit(4, 0));
        
        BackboneConjecture bbc{Glucose::mkLit(5,1)};
        
        testData.addEquivalence(eqc1);
        testData.addEquivalence(eqc2);
        testData.addBackbone(bbc);
        
        
        EquivalencyChecker checker;
        checker.createVariables(5);
        checker.finishedAddingRegularVariables();
        
        
        std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> heuristics;
        heuristics.reset(new std::vector<std::unique_ptr<RefinementHeuristic>>{});
        heuristics->push_back(std::move(mock));
        
        std::function<Var()> newVarFn = [&checker](){ return checker.createVariable(); };
        
        auto underTest = createDefaultRefinementStrategy(testData,
                                                         std::move(heuristics),
                                                         newVarFn);
        
        auto initDelta = underTest->init();
        EXPECT_EQ(initDelta->getNewClauses().size(), 6ull);
        EXPECT_EQ(initDelta->getAssumptionLiterals().size(), 6ull);
        EXPECT_EQ(initDelta->countEnabledClauses(), 6ull);
        
        // all equivalencies should be present
        checker.addClauses(initDelta->getNewClauses());
        EXPECT_TRUE(checker.isAllEquivalent(initDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(0, 1), Glucose::mkLit(1, 0), Glucose::mkLit(2, 0)}));
        EXPECT_TRUE(checker.isAllEquivalent(initDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(3, 1), Glucose::mkLit(4, 0)}));
        EXPECT_FALSE(checker.isEquivalent(initDelta->getAssumptionLiterals(),
                                          Glucose::mkLit(0, 1), Glucose::mkLit(4, 0)));
        
     
        auto sndDelta = underTest->refine();
        EXPECT_TRUE(sndDelta->getNewClauses().empty());
        EXPECT_EQ(sndDelta->getAssumptionLiterals().size(), 6ull);
        EXPECT_EQ(sndDelta->countEnabledClauses(), 6ull);
        
        // all equivalencies should remain present
        checker.addClauses(sndDelta->getNewClauses());
        EXPECT_TRUE(checker.isAllEquivalent(sndDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(0, 1), Glucose::mkLit(1, 0), Glucose::mkLit(2, 0)}));
        EXPECT_TRUE(checker.isAllEquivalent(sndDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(3, 1), Glucose::mkLit(4, 0)}));
        EXPECT_FALSE(checker.isEquivalent(sndDelta->getAssumptionLiterals(),
                                          Glucose::mkLit(0, 1), Glucose::mkLit(4, 0)));
    }
    
    
    TEST(RSARRefinementStrategy, refineWithTwoEquivalenceClasses) {
        auto mock = std::unique_ptr<MockHeuristic>(new MockHeuristic());
        EXPECT_CALL(*mock, beginRefinementStep()).Times(testing::Exactly(4));
        EXPECT_CALL(*mock, markRemovals(testing::Matcher<EquivalenceImplications&>(testing::_))).Times(testing::Exactly(8));
        EXPECT_CALL(*mock, markRemovals(testing::Matcher<Backbones&>(testing::_))).Times(testing::Exactly(0));
        
        auto fake = std::unique_ptr<FakeHeuristic>(new FakeHeuristic());
        fake->inStepNRemove(1, {1});
        fake->inStepNRemove(2, {3});
        fake->inStepNRemove(3, {0});
        
        Candy::Conjectures testData;
        
        EquivalenceConjecture eqc1;
        eqc1.addLit(Glucose::mkLit(0, 1));
        eqc1.addLit(Glucose::mkLit(1, 0));
        eqc1.addLit(Glucose::mkLit(2, 0));
        
        EquivalenceConjecture eqc2;
        eqc2.addLit(Glucose::mkLit(3, 1));
        eqc2.addLit(Glucose::mkLit(4, 0));
        
        testData.addEquivalence(eqc1);
        testData.addEquivalence(eqc2);
        
        
        EquivalencyChecker checker;
        checker.createVariables(4);
        checker.finishedAddingRegularVariables();
        
        
        std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> heuristics;
        heuristics.reset(new std::vector<std::unique_ptr<RefinementHeuristic>>{});
        heuristics->push_back(std::move(mock));
        heuristics->push_back(std::move(fake));
        
        std::function<Var()> newVarFn = [&checker](){ return checker.createVariable(); };
        
        auto underTest = createDefaultRefinementStrategy(testData,
                                                         std::move(heuristics),
                                                         newVarFn);
        
        auto initDelta = underTest->init();
        EXPECT_EQ(initDelta->getNewClauses().size(), 5ull);
        EXPECT_EQ(initDelta->getAssumptionLiterals().size(), 5ull);
        EXPECT_EQ(initDelta->countEnabledClauses(), 5ull);
        
        checker.addClauses(initDelta->getNewClauses());
        EXPECT_TRUE(checker.isAllEquivalent(initDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(0, 1), Glucose::mkLit(1, 0), Glucose::mkLit(2, 0)}));
        EXPECT_TRUE(checker.isAllEquivalent(initDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(3, 1), Glucose::mkLit(4, 0)}));
        EXPECT_FALSE(checker.isEquivalent(initDelta->getAssumptionLiterals(),
                                          Glucose::mkLit(0, 1), Glucose::mkLit(4, 0)));
        
        // Variable 1 removed
        auto sndDelta = underTest->refine();
        EXPECT_EQ(sndDelta->getNewClauses().size(), 1ull);
        EXPECT_EQ(sndDelta->getAssumptionLiterals().size(), 6ull);
        EXPECT_EQ(sndDelta->countEnabledClauses(), 4ull);
        
        checker.addClauses(sndDelta->getNewClauses());
        EXPECT_TRUE(checker.isAllEquivalent(sndDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(0, 1), Glucose::mkLit(2, 0)}));
        
        EXPECT_FALSE(checker.isAllEquivalent(sndDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(0, 1), Glucose::mkLit(1, 0)}));
        
        
        EXPECT_TRUE(checker.isAllEquivalent(sndDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(3, 1), Glucose::mkLit(4, 0)}));
        EXPECT_FALSE(checker.isEquivalent(sndDelta->getAssumptionLiterals(),
                                          Glucose::mkLit(0, 1), Glucose::mkLit(4, 0)));
        
        
        
        // Variable 3 removed
        auto thirdDelta = underTest->refine();
        EXPECT_EQ(thirdDelta->getNewClauses().size(), 0ull);
        EXPECT_EQ(thirdDelta->getAssumptionLiterals().size(), 6ull);
        EXPECT_EQ(thirdDelta->countEnabledClauses(), 2ull);
        
        checker.addClauses(thirdDelta->getNewClauses());
        EXPECT_TRUE(checker.isAllEquivalent(thirdDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(0, 1), Glucose::mkLit(2, 0)}));
        
        EXPECT_FALSE(checker.isAllEquivalent(thirdDelta->getAssumptionLiterals(),
                                             {Glucose::mkLit(0, 1), Glucose::mkLit(1, 0)}));
        
        
        EXPECT_FALSE(checker.isAllEquivalent(thirdDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(3, 1), Glucose::mkLit(4, 0)}));
        EXPECT_FALSE(checker.isEquivalent(thirdDelta->getAssumptionLiterals(),
                                          Glucose::mkLit(0, 1), Glucose::mkLit(4, 0)));
        
        // Variable 0 removed
        auto fourthDelta = underTest->refine();
        EXPECT_EQ(fourthDelta->getNewClauses().size(), 0ull);
        EXPECT_EQ(fourthDelta->getAssumptionLiterals().size(), 6ull);
        EXPECT_EQ(fourthDelta->countEnabledClauses(), 0ull);
    }
    
    
    TEST(RSARRefinementStrategy, refineWithBackbonesAndDeletionInInitial) {
        auto mock = std::unique_ptr<MockHeuristic>(new MockHeuristic());
        EXPECT_CALL(*mock, beginRefinementStep()).Times(testing::Exactly(2));
        EXPECT_CALL(*mock, markRemovals(testing::Matcher<EquivalenceImplications&>(testing::_))).Times(testing::Exactly(2));
        EXPECT_CALL(*mock, markRemovals(testing::Matcher<Backbones&>(testing::_))).Times(testing::Exactly(2));
        
        auto fake = std::unique_ptr<FakeHeuristic>(new FakeHeuristic());
        fake->inStepNRemove(0, {1});
        fake->inStepNRemove(1, {3});
        
        Candy::Conjectures testData;
        
        EquivalenceConjecture eqc1;
        eqc1.addLit(Glucose::mkLit(0, 1));
        eqc1.addLit(Glucose::mkLit(1, 0));
        eqc1.addLit(Glucose::mkLit(2, 0));
        
        BackboneConjecture bbc1{Glucose::mkLit(3,0)};
        
        testData.addEquivalence(eqc1);
        testData.addBackbone(bbc1);
        
        
        EquivalencyChecker checker;
        checker.createVariables(4);
        checker.finishedAddingRegularVariables();
        
        
        std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> heuristics;
        heuristics.reset(new std::vector<std::unique_ptr<RefinementHeuristic>>{});
        heuristics->push_back(std::move(mock));
        heuristics->push_back(std::move(fake));
        
        std::function<Var()> newVarFn = [&checker](){ return checker.createVariable(); };
        
        auto underTest = createDefaultRefinementStrategy(testData,
                                                         std::move(heuristics),
                                                         newVarFn);
        
        auto initDelta = underTest->init();
        EXPECT_EQ(initDelta->getNewClauses().size(), 3ull);
        EXPECT_EQ(initDelta->getAssumptionLiterals().size(), 3ull);
        EXPECT_EQ(initDelta->countEnabledClauses(), 3ull);
        
        
        // Variable 1 removed
        checker.addClauses(initDelta->getNewClauses());
        EXPECT_TRUE(checker.isAllEquivalent(initDelta->getAssumptionLiterals(),
                                            {Glucose::mkLit(0, 1), Glucose::mkLit(2, 0)}));
        EXPECT_TRUE(checker.isBackbones(initDelta->getAssumptionLiterals(), {Glucose::mkLit(3,0)}));
        EXPECT_FALSE(checker.isEquivalent(initDelta->getAssumptionLiterals(),
                                          Glucose::mkLit(0, 1), Glucose::mkLit(1, 0)));
        
        
        // Variable 3 removed
        auto sndDelta = underTest->refine();
        EXPECT_EQ(sndDelta->getNewClauses().size(), 0ull);
        EXPECT_EQ(sndDelta->getAssumptionLiterals().size(), 3ull);
        EXPECT_EQ(sndDelta->countEnabledClauses(), 2ull);
        
        checker.addClauses(sndDelta->getNewClauses());
        EXPECT_FALSE(checker.isBackbones(initDelta->getAssumptionLiterals(), {Glucose::mkLit(3,0)}));
        
    }
}
