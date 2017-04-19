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
#include <randomsimulation/Conjectures.h>
#include <rsar/ARSolver.h>
#include <rsar/Refinement.h>
#include <utils/FastRand.h>

#include "SolverMock.h"
#include "HeuristicsMock.h"
#include "TestUtils.h"

#include <iostream>
#include <string>
#include <queue>

namespace Candy {
    static std::unique_ptr<Conjectures> makeDummyConjectures() {
        auto result = std::unique_ptr<Conjectures>(new Conjectures());
        result->addBackbone(BackboneConjecture{mkLit(0, 1)});
        result->addBackbone(BackboneConjecture{mkLit(1, 1)});
        return result;
    }
    
    static void test_allAddedClausesDeactivated(SolverMock &mock) {
        auto eventLog = mock.mockctrl_getEventLog();
        auto nAddedClauses = std::count(eventLog.begin(),
                                        eventLog.end(),
                                        SolverMockEvent::ADD_CLAUSE);
        
        auto& lastAssumptionLits = mock.mockctrl_getLastAssumptionLits();
        
        EXPECT_EQ(lastAssumptionLits.size(), static_cast<size_t>(nAddedClauses));
        EXPECT_TRUE(std::all_of(lastAssumptionLits.begin(),
                                lastAssumptionLits.end(),
                                [](Lit l) { return !isActive(l); }));
    }

    struct ARSolverAndGlucoseMock {
        std::unique_ptr<ARSolver> arSolver;
        SolverMock* glucoseMock;
        CNFProblem minProblem;
    };
    
    CNFProblem createSimplestCNFProblem(Conjectures &conjectures) {
        Var maxVar = 0;
        for (auto&& equivalence : conjectures.getEquivalences()) {
            for (auto&& lit : equivalence) {
                maxVar = std::max(maxVar, var(lit));
            }
        }
        for (auto&& bbConj : conjectures.getBackbones()) {
            maxVar = std::max(maxVar, var(bbConj.getLit()));
        }
        
        CNFProblem result;
        result.readClause(mkLit(maxVar, 1));
        return result;
    }
    
    static ARSolverAndGlucoseMock createSimpleSetup(SimplificationHandlingMode mode) {
        auto solverMockPtr = createSolverMock();
        SolverMock& solverMock = *solverMockPtr;
        solverMock.mockctrl_setDefaultSolveResult(false);
        
        auto conjectures = makeDummyConjectures();
        CNFProblem simplemostProblem = createSimplestCNFProblem(*conjectures);
        
        auto underTestBuilder = createARSolverBuilder();
        underTestBuilder->withSolver(std::move(solverMockPtr));
        underTestBuilder->withSimplificationHandlingMode(mode);
        
        underTestBuilder->withConjectures(makeDummyConjectures());
        underTestBuilder->withMaxRefinementSteps(3);
        auto underTest = underTestBuilder->build();
        
        return ARSolverAndGlucoseMock{std::move(underTest), &solverMock, simplemostProblem};
    }
    
    
    static std::vector<SolverMockEvent> getEventLog(ARSolverAndGlucoseMock& underTest) {
        return underTest.glucoseMock->mockctrl_getEventLog();
    }
    
    TEST(RSARARSolver, temporal_init_simpCallTime_DISABLEsimp) {
        auto underTest = createSimpleSetup(SimplificationHandlingMode::DISABLE);
        underTest.arSolver->solve(underTest.minProblem);
        auto eventLog = getEventLog(underTest);
        
        EXPECT_TRUE(std::find(eventLog.begin(), eventLog.end(), SolverMockEvent::SIMPLIFY)
                    == eventLog.end());
    }
    
    TEST(RSARARSolver, temporal_init_simpCallTime_RESTRICTsimp) {
        auto underTest = createSimpleSetup(SimplificationHandlingMode::RESTRICT);
        underTest.arSolver->solve(underTest.minProblem);
        auto eventLog = getEventLog(underTest);
        
        EXPECT_TRUE(std::find(eventLog.begin(), eventLog.end(), SolverMockEvent::SIMPLIFY)
                    != eventLog.end());
        
        // Expected behaviour: add problem, add clauses, simplify, solve
        EXPECT_TRUE(occursOnlyAfter(eventLog, SolverMockEvent::SIMPLIFY, SolverMockEvent::ADD_PROBLEM));
        EXPECT_TRUE(occursBefore(eventLog, SolverMockEvent::ADD_CLAUSE, SolverMockEvent::SIMPLIFY));
        EXPECT_TRUE(occursOnlyBefore(eventLog, SolverMockEvent::SIMPLIFY, SolverMockEvent::SOLVE));
    }
    
    TEST(RSARARSolver, temporal_init_simpCallTime_FREEZEsimp) {
        auto underTest = createSimpleSetup(SimplificationHandlingMode::FREEZE);
        underTest.arSolver->solve(underTest.minProblem);
        auto eventLog = getEventLog(underTest);
        
        // Expected behaviour: add problem, freeze literals, add clauses, simplify, solve
        EXPECT_TRUE(occursOnlyAfter(eventLog, SolverMockEvent::SIMPLIFY, SolverMockEvent::ADD_PROBLEM));
        EXPECT_TRUE(occursBefore(eventLog, SolverMockEvent::ADD_CLAUSE, SolverMockEvent::SIMPLIFY));
        EXPECT_TRUE(occursOnlyBefore(eventLog, SolverMockEvent::SIMPLIFY, SolverMockEvent::SOLVE));
    }
    
    TEST(RSARARSolver, temporal_init_simpCallTime_FULLsimp) {
        auto underTest = createSimpleSetup(SimplificationHandlingMode::FULL);
        underTest.arSolver->solve(underTest.minProblem);
        auto eventLog = getEventLog(underTest);
        
        // Expected behaviour: add problem, simplify, add clauses, solve
        EXPECT_TRUE(occursOnlyAfter(eventLog, SolverMockEvent::SIMPLIFY, SolverMockEvent::ADD_PROBLEM));
        EXPECT_TRUE(occursOnlyBefore(eventLog, SolverMockEvent::SIMPLIFY, SolverMockEvent::ADD_CLAUSE));
        EXPECT_TRUE(occursOnlyBefore(eventLog, SolverMockEvent::SIMPLIFY, SolverMockEvent::SOLVE));
    }
    
    static void test_temporal_init_solveCallTime(SimplificationHandlingMode mode) {
        auto underTest = createSimpleSetup(mode);
        underTest.arSolver->solve(underTest.minProblem);
        auto eventLog = getEventLog(underTest);
        EXPECT_TRUE(occursOnlyAfter(eventLog, SolverMockEvent::SOLVE, SolverMockEvent::ADD_PROBLEM));
        EXPECT_TRUE(!occursBefore(eventLog, SolverMockEvent::SOLVE, SolverMockEvent::ADD_CLAUSE));
        if (std::find(eventLog.begin(), eventLog.end(), SolverMockEvent::SIMPLIFY) != eventLog.end()) {
            EXPECT_TRUE(occursOnlyAfter(eventLog, SolverMockEvent::SOLVE, SolverMockEvent::SIMPLIFY));
        }
    }
    
    TEST(RSARARSolver, temporal_init_solveCallTime_DISABLEsimp) {
        test_temporal_init_solveCallTime(SimplificationHandlingMode::DISABLE);
    }
    
    TEST(RSARARSolver, temporal_init_solveCallTime_RESTRICTsimp) {
        test_temporal_init_solveCallTime(SimplificationHandlingMode::RESTRICT);
    }
    
    TEST(RSARARSolver, temporal_init_solveCallTime_FREEZEsimp) {
        test_temporal_init_solveCallTime(SimplificationHandlingMode::FREEZE);
    }
    
    TEST(RSARARSolver, temporal_init_solveCallTime_FULLsimp) {
        test_temporal_init_solveCallTime(SimplificationHandlingMode::FULL);
    }
    
    
    TEST(RSARARSolver, forcesDeactivation_SimpleProblem) {
        auto underTest = createSimpleSetup(SimplificationHandlingMode::DISABLE);
        underTest.arSolver->solve(underTest.minProblem);
        // no heuristics employed, no conflict assumption lits indicated
        // by the solver, no eliminated vars => ARSolver needs to force
        // the additional clauses to be deactivated in the end
        
        test_allAddedClausesDeactivated(*underTest.glucoseMock);
    }
    
    
    static ARSolverAndGlucoseMock createSetup(SimplificationHandlingMode simplificationMode,
                                              std::unique_ptr<Conjectures> conjectures,
                                              const std::vector<std::vector<Var>> &deactivationsByStep,
                                              CNFProblem minProblem) {
        auto solverMockPtr = createSolverMock();
        SolverMock& solverMock = *solverMockPtr;
        solverMock.mockctrl_setDefaultSolveResult(false);
        
        auto fakeHeur = std::unique_ptr<FakeHeuristic>(new FakeHeuristic());
        for (size_t i = 0; i < deactivationsByStep.size(); ++i) {
            fakeHeur->inStepNRemove(i, deactivationsByStep[i]);
        }
        
        auto underTestBuilder = createARSolverBuilder();
        underTestBuilder->withSolver(std::move(solverMockPtr));
        underTestBuilder->withSimplificationHandlingMode(simplificationMode);
        underTestBuilder->withConjectures(std::move(conjectures));
        underTestBuilder->addRefinementHeuristic(std::move(fakeHeur));
        auto underTest = underTestBuilder->build();
        
        return ARSolverAndGlucoseMock{std::move(underTest), &solverMock, minProblem};
    }
    
    static ARSolverAndGlucoseMock createDefaultTestSetup() {
        auto conjectures = std::unique_ptr<Conjectures>(new Conjectures());
        conjectures->addEquivalence(createEquivalenceConjecture({
            mkLit(0, 1),
            mkLit(1, 0),
            mkLit(2, 1),
            mkLit(3, 0)}));
        conjectures->addEquivalence(createEquivalenceConjecture({
            mkLit(4, 1),
            mkLit(5, 0)}));
        int nVars = 6;
        CNFProblem minProblem;
        minProblem.readClause(mkLit(nVars, 1));
        return createSetup(SimplificationHandlingMode::DISABLE,
                           std::move(conjectures),
                           {{1}, {4}, {0, 3}},
                           minProblem);
    }
    
    TEST(RSARARSolver, ultimatelyTerminatesWhenUNSAT) {
        auto solverAndMock = createDefaultTestSetup();
        
        solverAndMock.glucoseMock->mockctrl_setDefaultSolveResult(false);
        
        bool result = solverAndMock.arSolver->solve(solverAndMock.minProblem);
        
        EXPECT_EQ(result, false);
        EXPECT_EQ(solverAndMock.glucoseMock->mockctrl_getAmountOfInvocations(), 3);
        test_allAddedClausesDeactivated(*solverAndMock.glucoseMock);
    }
    
    TEST(RSARARSolver, terminatesOnSAT) {
        auto solverAndMock = createDefaultTestSetup();
        
        solverAndMock.glucoseMock->mockctrl_setDefaultSolveResult(false);
        solverAndMock.glucoseMock->mockctrl_setResultInInvocation(1, true);
        
        bool result = solverAndMock.arSolver->solve(solverAndMock.minProblem);
        
        EXPECT_EQ(result, true);
        EXPECT_EQ(solverAndMock.glucoseMock->mockctrl_getAmountOfInvocations(), 2);
    }
    
    
    TEST(RSARARSolver, doesNotAddEliminatedVariablesViaApproximation) {
        auto solverAndMock = createDefaultTestSetup();
        solverAndMock.glucoseMock->mockctrl_setDefaultSolveResult(false);
        solverAndMock.glucoseMock->mockctrl_setEliminated(1, true);
        solverAndMock.glucoseMock->mockctrl_setEliminated(2, true);
        
        solverAndMock.arSolver->solve(solverAndMock.minProblem);
        
        EXPECT_FALSE(varOccursIn(solverAndMock.glucoseMock->mockctrl_getAddedClauses(), 1));
        EXPECT_FALSE(varOccursIn(solverAndMock.glucoseMock->mockctrl_getAddedClauses(), 2));
        
        EXPECT_TRUE(varOccursIn(solverAndMock.glucoseMock->mockctrl_getAddedClauses(), 0));
        EXPECT_TRUE(varOccursIn(solverAndMock.glucoseMock->mockctrl_getAddedClauses(), 3));
        EXPECT_TRUE(varOccursIn(solverAndMock.glucoseMock->mockctrl_getAddedClauses(), 4));
        EXPECT_TRUE(varOccursIn(solverAndMock.glucoseMock->mockctrl_getAddedClauses(), 5));
    }
    
    TEST(RSARARSolver, addedApproximationClausesRepresentEquivalencies) {
        auto solverAndMock = createDefaultTestSetup();
        solverAndMock.glucoseMock->mockctrl_setDefaultSolveResult(false);
        
        solverAndMock.glucoseMock->mockctrl_callOnSolve([&solverAndMock](int round) {
            auto checker = createEquivalencyChecker();
            
            auto maxNonAssumVar = solverAndMock.glucoseMock->mockctrl_getNonAssumptionVars().second;
            auto maxVar = solverAndMock.glucoseMock->mockctrl_getAssumptionVars().second;
            ASSERT_TRUE(maxNonAssumVar != -1);
            ASSERT_TRUE(maxVar != -1);
            
            checker->createVariables(maxNonAssumVar);
            checker->finishedAddingRegularVariables();
            checker->createVariables(maxVar);
            
            checker->addClauses(solverAndMock.glucoseMock->mockctrl_getAddedClauses());
            
            if (round == 0) {
                // see createDefaultTestSetup() for initial conjectures and mock heuristic
                // configuration
                EXPECT_TRUE(checker->isAllEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                    {mkLit(2, 1),
                                                        mkLit(3, 0),
                                                        mkLit(0, 1)}));
                EXPECT_TRUE(checker->isAllEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                    {mkLit(4, 0),
                                                        mkLit(5, 1)}));
                
                // var. 1 should be removed before the first call to solve
                EXPECT_FALSE(checker->isEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                 mkLit(2, 1),
                                                 mkLit(1, 0)));
                
                // the two "equivalency groups" should not be equivalent
                EXPECT_FALSE(checker->isEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                  mkLit(2, 1),
                                                  mkLit(4, 0)));
            }
            else if (round == 1) {
                EXPECT_TRUE(checker->isAllEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                    {mkLit(2, 1),
                                                        mkLit(3, 0),
                                                        mkLit(0, 1)}));

                
                // var. 1 should be removed before the first call to solve
                EXPECT_FALSE(checker->isEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                  mkLit(2, 1),
                                                  mkLit(1, 0)));
                
                // the two "equivalency groups" should not be equivalent
                EXPECT_FALSE(checker->isEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                  mkLit(2, 1),
                                                  mkLit(4, 0)));
                
                // the smaller "equivalency group" should not exist anymore
                EXPECT_FALSE(checker->isAllEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                    {mkLit(4, 0),
                                                        mkLit(5, 1)}));
            }
            else if (round == 3) {
                // everything should be deactivated by now
                test_allAddedClausesDeactivated(*(solverAndMock.glucoseMock));
            }
        });
        
        solverAndMock.arSolver->solve(solverAndMock.minProblem);
    }
    
    
    static void test_acceptanceTest(std::string filename,
                                    SimplificationHandlingMode simpMode,
                                    bool expectedResult,
                                    bool addHeuristics = true) {
        CNFProblem problem;
        problem.readDimacsFromFile(filename.c_str());
        ASSERT_FALSE(problem.getProblem().empty()) << "Could not read test problem file.";
        
        // Create random conjectures and a random deactivation sequence, "faking" the
        // results of random simulation. The resulting approximations are almost certainly unsatisfiable.
        // This means working with particularly bad conjectures, which is useful for testing and okay
        // since random simulation is not tested here.
        auto randomLiterals = pickLiterals(problem, 10);
        auto randomConjectures = createRandomConjectures(randomLiterals, problem);
        auto randomHeuristic = createRandomHeuristic(randomLiterals);
        
        
        auto solverBuilder = createARSolverBuilder();
        solverBuilder->withSimplificationHandlingMode(simpMode);
        solverBuilder->withConjectures(std::move(randomConjectures));
        solverBuilder->addRefinementHeuristic(std::move(randomHeuristic));
        
        auto solver = solverBuilder->build();
        auto result = solver->solve(problem);
        EXPECT_EQ(result, expectedResult);
    }
    
    static void test_acceptanceTest_problem_flat200(int n, SimplificationHandlingMode simpMode,
                                                    bool expectedResult,
                                                    bool addHeuristics = true) {
        std::string filename = "problems/flat200-" + std::to_string(n) + ".cnf";
        return test_acceptanceTest(filename, simpMode, expectedResult, addHeuristics);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_1_DISABLEsimp) {
        test_acceptanceTest_problem_flat200(1, SimplificationHandlingMode::DISABLE, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_3_DISABLEsimp) {
        test_acceptanceTest_problem_flat200(3, SimplificationHandlingMode::DISABLE, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_6_DISABLEsimp) {
        test_acceptanceTest_problem_flat200(6, SimplificationHandlingMode::DISABLE, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_1_RESTRICTsimp) {
        test_acceptanceTest_problem_flat200(1, SimplificationHandlingMode::RESTRICT, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_3_RESTRICTsimp) {
        test_acceptanceTest_problem_flat200(3, SimplificationHandlingMode::RESTRICT, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_6_RESTRICTsimp) {
        test_acceptanceTest_problem_flat200(6, SimplificationHandlingMode::RESTRICT, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_1_FREEZEsimp) {
        test_acceptanceTest_problem_flat200(1, SimplificationHandlingMode::FREEZE, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_3_FREEZEsimp) {
        test_acceptanceTest_problem_flat200(3, SimplificationHandlingMode::FREEZE, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_6_FREEZEsimp) {
        test_acceptanceTest_problem_flat200(6, SimplificationHandlingMode::FREEZE, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_1_FULLsimp) {
        test_acceptanceTest_problem_flat200(1, SimplificationHandlingMode::FULL, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_3_FULLsimp) {
        test_acceptanceTest_problem_flat200(3, SimplificationHandlingMode::FULL, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_6_FULLsimp) {
        test_acceptanceTest_problem_flat200(6, SimplificationHandlingMode::FULL, true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_1_DISABLEsimp_onlyInternalHeur) {
        test_acceptanceTest_problem_flat200(1, SimplificationHandlingMode::DISABLE, true, false);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_3_DISABLEsimp_onlyInternalHeur) {
        test_acceptanceTest_problem_flat200(3, SimplificationHandlingMode::DISABLE, true, false);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_6_DISABLEsimp_onlyInternalHeur) {
        test_acceptanceTest_problem_flat200(6, SimplificationHandlingMode::DISABLE, true, false);
    }
    
    static void test_acceptanceTest_problem_6s(SimplificationHandlingMode simpMode,
                                               bool expectedResult,
                                               bool addHeuristics = true) {
        return test_acceptanceTest(std::string("problems/6s33.cnf"),
                                   simpMode,
                                   expectedResult,
                                   addHeuristics);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_6s33_DISABLEsimp) {
        test_acceptanceTest_problem_6s(SimplificationHandlingMode::DISABLE, false);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_6s33_RESTRICTsimp) {
        test_acceptanceTest_problem_6s(SimplificationHandlingMode::RESTRICT, false);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_6s33_FREEZEsimp) {
        test_acceptanceTest_problem_6s(SimplificationHandlingMode::FREEZE, false);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_6s33_FULLsimp) {
        test_acceptanceTest_problem_6s(SimplificationHandlingMode::FULL, false);
    }
}

