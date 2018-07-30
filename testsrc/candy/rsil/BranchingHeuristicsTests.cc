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
#include <gmock/gmock.h>

#include <candy/rsil/BranchingHeuristics.h>
#include <candy/testutils/TestUtils.h>
#include <candy/core/Solver.h>
#include <candy/core/Trail.h>
#include <candy/randomsimulation/RandomSimulator.h>
#include <candy/gates/GateAnalyzer.h>
#include <candy/rsar/Heuristics.h>
#include <candy/frontend/RSILSolverBuilder.h>

namespace Candy {
    
    using TestedRSILBranchingHeuristic = RSILBranchingHeuristic3;
    using TestedRSILVanishingBranchingHeuristic = RSILVanishingBranchingHeuristic3;
    using TestedRSILBudgetBranchingHeuristic = RSILBudgetBranchingHeuristic3;
    
    template<class Heuristic>
    static void test_uninitializedHeuristicReturnsUndefForMinInput() {
        Heuristic underTest;

        Trail trail(1);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(1_L);
        auto result = underTest.getAdvice(trail);

        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, uninitializedHeuristicReturnsUndefForMinInput) {
        test_uninitializedHeuristicReturnsUndefForMinInput<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, uninitializedHeuristicReturnsUndefForMinInput) {
        test_uninitializedHeuristicReturnsUndefForMinInput<TestedRSILVanishingBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, uninitializedHeuristicReturnsUndefForMinInput) {
        test_uninitializedHeuristicReturnsUndefForMinInput<TestedRSILBudgetBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_emptyInitializedHeuristicReturnsUndefForMinInput() {
        Conjectures empty{};
        Heuristic underTest;
        
        Trail trail(1);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(1_L);
        auto result = underTest.getAdvice(trail);

        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, emptyInitializedHeuristicReturnsUndefForMinInput) {
        test_emptyInitializedHeuristicReturnsUndefForMinInput<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, emptyInitializedHeuristicReturnsUndefForMinInput) {
        test_emptyInitializedHeuristicReturnsUndefForMinInput<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, emptyInitializedHeuristicReturnsUndefForMinInput) {
        test_emptyInitializedHeuristicReturnsUndefForMinInput<TestedRSILBudgetBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesAdviceForSingleEquivalence(Heuristic underTest) {
        Trail trail(5);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(2_L);
        underTest.grow(5);
        auto result = underTest.getAdvice(trail);

        ASSERT_NE(result, lit_Undef);
        EXPECT_EQ(result, mkLit(2, 0));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBranchingHeuristic3 heuristic(std::move(testData)); //, backbonesEnabled, RSARHeuristic.get(), filterOnlyBackbones);
        test_givesAdviceForSingleEquivalence<TestedRSILBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILVanishingBranchingHeuristic3 heuristic(std::move(testData));
        test_givesAdviceForSingleEquivalence<TestedRSILVanishingBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBudgetBranchingHeuristic3 heuristic(std::move(testData));
        test_givesAdviceForSingleEquivalence<TestedRSILBudgetBranchingHeuristic>(std::move(heuristic));
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfAssigned(Heuristic underTest) {
        Trail trail(5);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(1_L);
        trail.uncheckedEnqueue(2_L);
        trail.uncheckedEnqueue(3_L);
        underTest.grow(5);
        auto result = underTest.getAdvice(trail);

        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBranchingHeuristic3 heuristic(std::move(testData));
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILVanishingBranchingHeuristic3 heuristic(std::move(testData));
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILVanishingBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBudgetBranchingHeuristic3 heuristic(std::move(testData));
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILBudgetBranchingHeuristic>(std::move(heuristic));
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision(Heuristic underTest) {
        Trail trail(5);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(2_L);
        underTest.grow(5);
        underTest.setDecisionVar(3_V, false);
        auto result = underTest.getAdvice(trail);

        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBranchingHeuristic3 heuristic(std::move(testData));
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILVanishingBranchingHeuristic3 heuristic(std::move(testData));
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILVanishingBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBudgetBranchingHeuristic3 heuristic(std::move(testData));
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILBudgetBranchingHeuristic>(std::move(heuristic));
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfIrrelevant(Heuristic underTest) {
        Trail trail(5);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(5_L);
        underTest.grow(5);
        auto result = underTest.getAdvice(trail);

        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBranchingHeuristic3 heuristic(std::move(testData));
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILVanishingBranchingHeuristic3 heuristic(std::move(testData));
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILVanishingBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        RSILBudgetBranchingHeuristic3 heuristic(std::move(testData));
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILBudgetBranchingHeuristic>(std::move(heuristic));
    }
    
    template<class Heuristic>
    static void test_givesAdviceForSingleEquivalenceSize3(Heuristic underTest) {
        Trail trail(5);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(4_L);
        underTest.grow(5);
        auto result = underTest.getAdvice(trail);

        ASSERT_NE(result, lit_Undef);        
        EXPECT_TRUE((result == mkLit(1, 0)) || (result == mkLit(2,0)));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        RSILBranchingHeuristic3 heuristic(std::move(testData));
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        RSILVanishingBranchingHeuristic3 heuristic(std::move(testData));
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILVanishingBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        RSILBudgetBranchingHeuristic3 heuristic(std::move(testData));
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILBudgetBranchingHeuristic>(std::move(heuristic));
    }
    
    template<class Heuristic>
    static void test_travelsUpTrail(Heuristic underTest) {
        Trail trail(5);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(4_L);
        trail.uncheckedEnqueue(5_L);
        underTest.grow(5);
        auto result = underTest.getAdvice(trail);

        ASSERT_NE(result, lit_Undef);        
        EXPECT_TRUE((result == mkLit(1, 0)) || (result == mkLit(2,0)));
    }
    
    TEST(RSILBranchingHeuristicsTests, travelsUpTrail) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        RSILBranchingHeuristic3 heuristic(std::move(testData));
        test_travelsUpTrail<TestedRSILBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, travelsUpTrail) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        RSILVanishingBranchingHeuristic3 heuristic(std::move(testData));
        test_travelsUpTrail<TestedRSILVanishingBranchingHeuristic>(std::move(heuristic));
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, travelsUpTrail) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        RSILBudgetBranchingHeuristic3 heuristic(std::move(testData));
        test_travelsUpTrail<TestedRSILBudgetBranchingHeuristic>(std::move(heuristic));
    }
    
    namespace {
        class MockRSARHeuristic : public RefinementHeuristic {
        public:
            void beginRefinementStep() {} ;
            void markRemovals(EquivalenceImplications&) {};
            void markRemovals(Backbones&) {};
            MOCK_METHOD2(probe, bool(Var, bool));
        };
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForFilteredVariable) {
        MockRSARHeuristic* mockRSARHeuristic = new MockRSARHeuristic;
        
        EXPECT_CALL(*mockRSARHeuristic, probe(testing::Ge(4), testing::_)).WillRepeatedly(testing::Return(false));
        EXPECT_CALL(*mockRSARHeuristic, probe(testing::Le(3), testing::_)).WillRepeatedly(testing::Return(true));
        
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        testData.addEquivalence(EquivalenceConjecture{{mkLit(4, 0), mkLit(5,1)}});

        TestedRSILBranchingHeuristic underTest(std::move(testData), false, mockRSARHeuristic);

        Trail trail(6);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(4_L);
        underTest.grow(6);
        auto result = underTest.getAdvice(trail);
        
        ASSERT_EQ(result, lit_Undef);
        
        trail.cancelUntil(0);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(5_L);
        result = underTest.getAdvice(trail);
        
        ASSERT_EQ(result, mkLit(5,0));

        delete mockRSARHeuristic;
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoBackboneAdviceWhenBackbonesDeactivated) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(5,1), mkLit(2, 0)}});
        testData.addBackbone(BackboneConjecture {mkLit(3,0)});
        testData.addBackbone(BackboneConjecture {mkLit(4,1)});

        TestedRSILBranchingHeuristic underTest(std::move(testData), false);
        
        // backbone used here if backbone-usage would be activated:
        EXPECT_EQ(underTest.getSignAdvice(mkLit(3,0)), mkLit(3,0));
        
        EXPECT_EQ(underTest.getSignAdvice(mkLit(1,0)), mkLit(1,0));
        EXPECT_EQ(underTest.getSignAdvice(mkLit(3,1)), mkLit(3,1));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesBackboneAdviceWhenBackbonesActivated) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(5,1), mkLit(2, 0)}});
        testData.addBackbone(BackboneConjecture {mkLit(3,0)});
        testData.addBackbone(BackboneConjecture {mkLit(4,1)});

        TestedRSILBranchingHeuristic underTest(std::move(testData), true);
        
        // backbone used here:
        EXPECT_EQ(underTest.getSignAdvice(mkLit(3,0)), mkLit(3,1));
        
        EXPECT_EQ(underTest.getSignAdvice(mkLit(3,1)), mkLit(3,1));
        EXPECT_EQ(underTest.getSignAdvice(mkLit(1,0)), mkLit(1,0));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, isFullyActiveInFirstPeriod) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});

        TestedRSILVanishingBranchingHeuristic underTest(std::move(testData));

        Trail trail(5);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(2_L);
        underTest.grow(5);
        
        int calls = 0;
        for (int i = 0; i < 100; ++i) {
            auto result = underTest.getAdvice(trail);
            ++calls;
            EXPECT_NE(result, lit_Undef) << "Unexpected undef at call " << calls;
        }
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, activityMatchesExpectedDistribution) {
        Conjectures testData;

        const unsigned long long halfLife = 1000ull;
        const int stages = 20;

        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});

        TestedRSILVanishingBranchingHeuristic underTest(std::move(testData), false, nullptr, false, halfLife);

        Trail trail(5);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(1_L);
        trail.uncheckedEnqueue(~2_L);
        underTest.grow(5);
        
        std::unordered_map<uint8_t, double> distribution;
        std::unordered_map<uint8_t, double> referenceDistribution;
        
        for (int stage = 0; stage < stages; ++stage) {
            uint32_t definedResultCounter = 0;
            for (auto i = decltype(halfLife){0}; i < halfLife; ++i) {
                Lit result = underTest.getAdvice(trail);
                definedResultCounter += (result == lit_Undef) ? 0 : 1;
            }
            distribution[stage] = static_cast<double>(definedResultCounter) / static_cast<double>(halfLife);
            referenceDistribution[stage] = 1.0f/static_cast<double>(1 << stage);
        }
        
        // simple Kolomogorov-Smirnov test
        double maxAbsDiff = getMaxAbsDifference(distribution, referenceDistribution);
        EXPECT_LE(maxAbsDiff, 0.05f);
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, producesUndefWhenBudgetIsDepleted) {
        const uint64_t budget = 4ull;
        
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});

        TestedRSILBudgetBranchingHeuristic underTest(std::move(testData), false, nullptr, false, budget);
        
        Trail trail(5);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(2_L);
        underTest.grow(5);

        for (uint64_t i = 0; i <= budget+2; ++i) {
            Lit result = underTest.getAdvice(trail);
            if (i < budget) {
                ASSERT_NE(result, lit_Undef) << "Budgets should not have been depleted in round " << i+1;
            }
            else {
                ASSERT_EQ(result, lit_Undef) << "Budgets should have been depleted in round " << i+1;
            }
        }
        
        trail.cancelUntil(0);
        trail.newDecisionLevel();
        trail.uncheckedEnqueue(3_L);

        Lit result = underTest.getAdvice(trail);
        ASSERT_NE(result, lit_Undef) << "The back implication's budget should not be depleted in this test";
    }

    static void test_acceptanceTest(std::string filename, RSILMode mode, bool expectedResult, unsigned int size = 3) {
        CNFProblem problem;
        problem.readDimacsFromFile(filename.c_str());
        ASSERT_FALSE(problem.getProblem().empty()) << "Could not read test problem file.";
        
        GateAnalyzer gateAnalyzer {problem};
        gateAnalyzer.analyze();
        auto randomSimulator = createDefaultRandomSimulator(gateAnalyzer);
        auto conjectures = randomSimulator->run(1ull << 16);

        RSILArguments args = { true, RSILMode::UNRESTRICTED, 1ull << 16, 0, false, 0, false, 0.0, false };

        RSILSolverBuilder builder;
        builder.withConjectures(conjectures);
        builder.withArgs(args);
        builder.withMode(mode);
        builder.withSize(size);

        CandySolverInterface* solver = builder.build();
        solver->addClauses(problem);
        EXPECT_TRUE((expectedResult ? l_True : l_False) == solver->solve());
    }
    
    TEST(RSILBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize3) {
        test_acceptanceTest("problems/6s33.cnf", RSILMode::UNRESTRICTED, false);
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize3) {
        test_acceptanceTest("problems/6s33.cnf", RSILMode::VANISHING, false);
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize3) {
        test_acceptanceTest("problems/6s33.cnf", RSILMode::IMPLICATIONBUDGETED, false);
    }

    TEST(RSILBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize2) {
        test_acceptanceTest("problems/6s33.cnf", RSILMode::UNRESTRICTED, false, 2);
    }

    TEST(RSILVanishingBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize2) {
        test_acceptanceTest("problems/6s33.cnf", RSILMode::VANISHING, false, 2);
    }

    TEST(RSILBudgetBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize2) {
        test_acceptanceTest("problems/6s33.cnf", RSILMode::IMPLICATIONBUDGETED, false, 2);
    }
}
