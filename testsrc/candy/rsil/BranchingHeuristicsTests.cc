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
#include <candy/randomsimulation/RandomSimulator.h>
#include <candy/gates/GateAnalyzer.h>
#include <candy/rsar/Heuristics.h>
#include <candy/frontend/RSILFrontend.h>

namespace Candy {
    
    using TestedRSILBranchingHeuristic = RSILBranchingHeuristic3;
    using TestedRSILVanishingBranchingHeuristic = RSILVanishingBranchingHeuristic3;
    using TestedRSILBudgetBranchingHeuristic = RSILBudgetBranchingHeuristic3;
    
    template<class Heuristic>
    static void test_uninitializedHeuristicReturnsUndefForMinInput() {
        Heuristic underTest;
        auto result = underTest.getAdvice({mkLit(0)}, 1ull, {0}, {}, {0});
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
        
        auto result = underTest.getAdvice({mkLit(0)}, 1ull, {0}, {}, {0});
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
    static void test_givesAdviceForSingleEquivalence() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.probHalfLife = 1000ull;
        Heuristic underTest;
        
        typename CandyDefaultSolverTypes::TrailType testTrail {mkLit(1,0)};
        typename CandyDefaultSolverTypes::TrailLimType testTrailLim {0};
        typename CandyDefaultSolverTypes::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        typename CandyDefaultSolverTypes::AssignsType testAssigns {l_True, l_False, l_Undef, l_Undef, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
        ASSERT_NE(result, lit_Undef);
        EXPECT_EQ(result, mkLit(2, 0));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        test_givesAdviceForSingleEquivalence<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        test_givesAdviceForSingleEquivalence<TestedRSILVanishingBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesAdviceForSingleEquivalence) {
        test_givesAdviceForSingleEquivalence<TestedRSILBudgetBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfAssigned() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.probHalfLife = 1000ull;
        Heuristic underTest;
        
        typename CandyDefaultSolverTypes::TrailType testTrail {mkLit(1,0)};
        typename CandyDefaultSolverTypes::TrailLimType testTrailLim {0};
        typename CandyDefaultSolverTypes::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        typename CandyDefaultSolverTypes::AssignsType testAssigns {l_True, l_False, l_True, l_Undef, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILVanishingBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfAssigned) {
        test_givesNoAdviceForSingleEquivalenceIfAssigned<TestedRSILBudgetBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.probHalfLife = 1000ull;
        Heuristic underTest;
        
        typename CandyDefaultSolverTypes::TrailType testTrail {mkLit(1,0)};
        typename CandyDefaultSolverTypes::TrailLimType testTrailLim {0};
        typename CandyDefaultSolverTypes::DecisionType testDecisionVars {1, 1, 0, 1, 1};
        typename CandyDefaultSolverTypes::AssignsType testAssigns {l_True, l_False, l_True, l_Undef, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILVanishingBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision) {
        test_givesNoAdviceForSingleEquivalenceIfNotEligibleForDecision<TestedRSILBudgetBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesNoAdviceForSingleEquivalenceIfIrrelevant() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.probHalfLife = 1000ull;
        Heuristic underTest;
        
        typename CandyDefaultSolverTypes::TrailType testTrail {mkLit(4,0)};
        typename CandyDefaultSolverTypes::TrailLimType testTrailLim {0};
        typename CandyDefaultSolverTypes::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        typename CandyDefaultSolverTypes::AssignsType testAssigns {l_True, l_True, l_Undef, l_Undef, l_False};
        
        auto result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
        EXPECT_EQ(result, lit_Undef);
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILVanishingBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesNoAdviceForSingleEquivalenceIfIrrelevant) {
        test_givesNoAdviceForSingleEquivalenceIfIrrelevant<TestedRSILBudgetBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_givesAdviceForSingleEquivalenceSize3() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.probHalfLife = 1000ull;
        Heuristic underTest;
        
        typename CandyDefaultSolverTypes::TrailType testTrail {mkLit(3,0)};
        typename CandyDefaultSolverTypes::TrailLimType testTrailLim {0};
        typename CandyDefaultSolverTypes::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        typename CandyDefaultSolverTypes::AssignsType testAssigns {l_Undef, l_Undef, l_Undef, l_False, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
        ASSERT_NE(result, lit_Undef);
        
        EXPECT_TRUE((result == mkLit(1, 0)) || (result == mkLit(2,0)));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILVanishingBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, givesAdviceForSingleEquivalenceSize3) {
        test_givesAdviceForSingleEquivalenceSize3<TestedRSILBudgetBranchingHeuristic>();
    }
    
    template<class Heuristic>
    static void test_travelsUpTrail() {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(3,1), mkLit(2, 0)}});
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.probHalfLife = 1000ull;
        Heuristic underTest;
        
        typename CandyDefaultSolverTypes::TrailType testTrail {mkLit(3,0), mkLit(4,0)};
        typename CandyDefaultSolverTypes::TrailLimType testTrailLim {0};
        typename CandyDefaultSolverTypes::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        typename CandyDefaultSolverTypes::AssignsType testAssigns {l_Undef, l_Undef, l_Undef, l_False, l_False};
        
        auto result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
        ASSERT_NE(result, lit_Undef);
        
        EXPECT_TRUE((result == mkLit(1, 0)) || (result == mkLit(2,0)));
    }
    
    TEST(RSILBranchingHeuristicsTests, travelsUpTrail) {
        test_travelsUpTrail<TestedRSILBranchingHeuristic>();
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, travelsUpTrail) {
        test_travelsUpTrail<TestedRSILVanishingBranchingHeuristic>();
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, travelsUpTrail) {
        test_travelsUpTrail<TestedRSILBudgetBranchingHeuristic>();
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

        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.filterByRSARHeuristic = true;
        RSILBranchingHeuristic3::defaultParameters.RSARHeuristic.reset(mockRSARHeuristic);

        TestedRSILBranchingHeuristic underTest;
                
        CandyDefaultSolverTypes::TrailType testTrail {mkLit(3,0)};
        CandyDefaultSolverTypes::TrailLimType testTrailLim {0};
        CandyDefaultSolverTypes::DecisionType testDecisionVars {1, 1, 1, 1, 1, 1};
        CandyDefaultSolverTypes::AssignsType testAssigns {l_Undef, l_Undef, l_Undef, l_False, l_False, l_Undef};
        
        auto result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
        
        ASSERT_EQ(result, lit_Undef);
        
        testTrail = {mkLit(4,0)};
        
        result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
        
        ASSERT_EQ(result, mkLit(5,0));
    }
    
    TEST(RSILBranchingHeuristicsTests, givesNoBackboneAdviceWhenBackbonesDeactivated) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(5,1), mkLit(2, 0)}});
        testData.addBackbone(BackboneConjecture {mkLit(3,0)});
        testData.addBackbone(BackboneConjecture {mkLit(4,1)});
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.probHalfLife = 1000ull;
        TestedRSILBranchingHeuristic underTest;
        
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
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.backbonesEnabled = true;
        TestedRSILBranchingHeuristic underTest;
        
        // backbone used here:
        EXPECT_EQ(underTest.getSignAdvice(mkLit(3,0)), mkLit(3,1));
        
        EXPECT_EQ(underTest.getSignAdvice(mkLit(3,1)), mkLit(3,1));
        EXPECT_EQ(underTest.getSignAdvice(mkLit(1,0)), mkLit(1,0));
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, isFullyActiveInFirstPeriod) {
        Conjectures testData;
        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.probHalfLife = 1000ull;
        TestedRSILVanishingBranchingHeuristic underTest;
        
        CandyDefaultSolverTypes::TrailType testTrail {mkLit(1,0)};
        CandyDefaultSolverTypes::TrailLimType testTrailLim {0};
        CandyDefaultSolverTypes::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        CandyDefaultSolverTypes::AssignsType testAssigns {l_True, l_False, l_Undef, l_Undef, l_Undef};
        
        int calls = 0;
        for (int i = 0; i < 100; ++i) {
            auto result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
            ++calls;
            EXPECT_NE(result, lit_Undef) << "Unexpected undef at call " << calls;
        }
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, activityMatchesExpectedDistribution) {        
        Conjectures testData;

        const unsigned long long halfLife = 1000ull;
        const int stages = 20;

        testData.addEquivalence(EquivalenceConjecture{{mkLit(1, 0), mkLit(2,1)}});
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.probHalfLife = halfLife;
        TestedRSILVanishingBranchingHeuristic underTest;
        
        CandyDefaultSolverTypes::TrailType testTrail {mkLit(1,0)};
        CandyDefaultSolverTypes::TrailLimType testTrailLim {0};
        CandyDefaultSolverTypes::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        CandyDefaultSolverTypes::AssignsType testAssigns {l_True, l_False, l_Undef, l_Undef, l_Undef};
        
        std::unordered_map<uint8_t, double> distribution;
        std::unordered_map<uint8_t, double> referenceDistribution;
        
        for (int stage = 0; stage < stages; ++stage) {
            uint32_t definedResultCounter = 0;
            for (auto i = decltype(halfLife){0}; i < halfLife; ++i) {
                Lit result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
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
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(testData);
        RSILBranchingHeuristic3::defaultParameters.initialBudget = budget;
        TestedRSILBudgetBranchingHeuristic underTest;
        
        CandyDefaultSolverTypes::TrailType testTrail {mkLit(1,0)};
        CandyDefaultSolverTypes::TrailLimType testTrailLim {0};
        CandyDefaultSolverTypes::DecisionType testDecisionVars {1, 1, 1, 1, 1};
        CandyDefaultSolverTypes::AssignsType testAssigns {l_True, l_False, l_Undef, l_Undef, l_Undef};
        
        for (uint64_t i = 0; i <= budget+2; ++i) {
            Lit result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
            if (i < budget) {
                ASSERT_NE(result, lit_Undef) << "Budgets should not have been depleted in round " << i+1;
            }
            else {
                ASSERT_EQ(result, lit_Undef) << "Budgets should have been depleted in round " << i+1;
            }
        }
        
        testTrail[0] = mkLit(2, 0);
        testAssigns[1] = l_Undef;
        testAssigns[2] = l_False;
        Lit result = underTest.getAdvice(testTrail, testTrail.size(), testTrailLim, testAssigns, testDecisionVars);
        ASSERT_NE(result, lit_Undef) << "The back implication's budget should not be depleted in this test";
    }
    
    template<class Heuristic>
    static void test_acceptanceTest(std::string filename, bool expectedResult) {
        CNFProblem problem;
        problem.readDimacsFromFile(filename.c_str());
        ASSERT_FALSE(problem.getProblem().empty()) << "Could not read test problem file.";
        
        GateAnalyzer gateAnalyzer {problem};
        gateAnalyzer.analyze();
        auto randomSimulator = createDefaultRandomSimulator(gateAnalyzer);
        auto conjectures = randomSimulator->run(1ull << 16);

        RSILArguments args = { true, RSILMode::UNRESTRICTED, 1ull << 16, 0, false, 0, false, 0.0, false };
        (RSILSolverFactory<RSILBranchingHeuristic3>()).setRSILArgs(conjectures, args);

        Solver<Heuristic> solver;
        solver.addClauses(problem);
        EXPECT_TRUE((expectedResult ? l_True : l_False) == solver.solve());
    }
    
    TEST(RSILBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize3) {
        test_acceptanceTest<RSILBranchingHeuristic3>("problems/6s33.cnf", false);
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize3) {
        test_acceptanceTest<RSILVanishingBranchingHeuristic3>("problems/6s33.cnf", false);
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize3) {
        test_acceptanceTest<RSILBudgetBranchingHeuristic3>("problems/6s33.cnf", false);
    }
    
    TEST(RSILBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize2) {
        test_acceptanceTest<RSILBranchingHeuristic2>("problems/6s33.cnf", false);
    }
    
    TEST(RSILVanishingBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize2) {
        test_acceptanceTest<RSILVanishingBranchingHeuristic2>("problems/6s33.cnf", false);
    }
    
    TEST(RSILBudgetBranchingHeuristicsTests, acceptanceTest_problem_6s33_adviceSize2) {
        test_acceptanceTest<RSILBudgetBranchingHeuristic2>("problems/6s33.cnf", false);
    }
}
