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
#include <candy/rsar/ARSolver.h>
#include <candy/rsar/Refinement.h>
#include <candy/utils/FastRand.h>
#include <candy/utils/MemUtils.h>

#include "candy/frontend/CandyBuilder.h"
#include "SolverMock.h"
#include "HeuristicsMock.h"
#include "TestUtils.h"

#include <iostream>
#include <string>
#include <queue>

namespace Candy {
    static std::unique_ptr<Conjectures> makeDummyConjectures() {
        auto result = std::unique_ptr<Conjectures>(new Conjectures());
        result->addBackbone(BackboneConjecture{Lit(0, 1)});
        result->addBackbone(BackboneConjecture{Lit(1, 1)});
        return result;
    }
    
    static void test_allAddedClausesDeactivated(SolverMock &mock) {
        auto eventLog = mock.mockctrl_getEventLog();
        auto& lastAssumptionLits = mock.mockctrl_getLastAssumptionLits();
        
        EXPECT_TRUE(std::all_of(lastAssumptionLits.begin(),
                                lastAssumptionLits.end(),
                                [](Lit l) { return !isActive(l); }));
    }

    struct ARSolverAndGlucoseMock {
        ARSolver* arSolver;
        SolverMock* glucoseMock;
        std::unique_ptr<CNFProblem> minProblem;
    };
    
    std::unique_ptr<CNFProblem> createSimplestCNFProblem(Conjectures &conjectures) {
        Var maxVar = 0;
        for (auto&& equivalence : conjectures.getEquivalences()) {
            for (auto&& lit : equivalence) {
                maxVar = std::max(maxVar, lit.var());
            }
        }
        for (auto&& bbConj : conjectures.getBackbones()) {
            maxVar = std::max(maxVar, bbConj.getLit().var());
        }
        
        std::unique_ptr<CNFProblem> result = backported_std::make_unique<CNFProblem>();
        result->readClause({ Lit(maxVar, 1) });
        return result;
    }
    
    static ARSolverAndGlucoseMock createSimpleSetup() {
    	SolverMock* solverMock = new SolverMock();
        solverMock->mockctrl_setDefaultSolveResult(false);
        
        auto conjectures = makeDummyConjectures();
        std::unique_ptr<CNFProblem> simplemostProblem = createSimplestCNFProblem(*conjectures);
        
        RSAROptions::opt_rsar_maxRefinementSteps = 3;

        ARSolver* underTest = new ARSolver(makeDummyConjectures(), solverMock);
        
        return ARSolverAndGlucoseMock{underTest, solverMock, std::move(simplemostProblem)};
    }
    
    
    static std::vector<SolverMockEvent> getEventLog(ARSolverAndGlucoseMock& underTest) {
        return underTest.glucoseMock->mockctrl_getEventLog();
    }
    
    TEST(RSARARSolver, temporal_init_simpCallTime) {
        auto underTest = createSimpleSetup();
        underTest.arSolver->init(*underTest.minProblem);
        underTest.arSolver->solve();
        auto eventLog = getEventLog(underTest);
        EXPECT_TRUE(occursOnlyBefore(eventLog, SolverMockEvent::ADD_PROBLEM, SolverMockEvent::SOLVE));
    }
    
    TEST(RSARARSolver, temporal_init_solveCallTime) {
        auto underTest = createSimpleSetup();
        underTest.arSolver->init(*underTest.minProblem);
        underTest.arSolver->solve();
        auto eventLog = getEventLog(underTest);
        EXPECT_TRUE(occursOnlyAfter(eventLog, SolverMockEvent::SOLVE, SolverMockEvent::ADD_PROBLEM));
    }
    
    
   TEST(RSARARSolver, forcesDeactivation_SimpleProblem) {
       auto underTest = createSimpleSetup();
       underTest.arSolver->init(*underTest.minProblem);
       underTest.arSolver->solve();
       // no heuristics employed, no conflict assumption lits indicated
       // by the solver, no eliminated vars => ARSolver needs to force
       // the additional clauses to be deactivated in the end
       test_allAddedClausesDeactivated(*underTest.glucoseMock);
   }
    
    
    static ARSolverAndGlucoseMock createSetup(std::unique_ptr<Conjectures> conjectures,
                                              const std::vector<std::vector<Var>> &deactivationsByStep,
                                              std::unique_ptr<CNFProblem> minProblem) {
    	SolverMock* solverMock = new SolverMock();
        solverMock->mockctrl_setDefaultSolveResult(false);
        
        auto fakeHeur = std::unique_ptr<FakeHeuristic>(new FakeHeuristic());
        for (size_t i = 0; i < deactivationsByStep.size(); ++i) {
            fakeHeur->inStepNRemove(i, deactivationsByStep[i]);
        }
        
        ARSolver* underTest = new ARSolver(std::move(conjectures), std::move(fakeHeur));
        
        return ARSolverAndGlucoseMock{underTest, solverMock, std::move(minProblem)};
    }
    
    static ARSolverAndGlucoseMock createDefaultTestSetup() {
        auto conjectures = std::unique_ptr<Conjectures>(new Conjectures());
        conjectures->addEquivalence(createEquivalenceConjecture({~1_L, 2_L, ~3_L, 4_L}));
        conjectures->addEquivalence(createEquivalenceConjecture({~5_L, 6_L}));
        std::unique_ptr<CNFProblem> minProblem = backported_std::make_unique<CNFProblem>(); 
        minProblem->readClause({~7_L});
        return createSetup(std::move(conjectures), {{1}, {4}, {0, 3}}, std::move(minProblem));
    }
    
    TEST(RSARARSolver, DISABLED_ultimatelyTerminatesWhenUNSAT) {
        auto solverAndMock = createDefaultTestSetup();
        
        solverAndMock.glucoseMock->mockctrl_setDefaultSolveResult(false);
        
        solverAndMock.arSolver->init(*solverAndMock.minProblem);
        lbool result = solverAndMock.arSolver->solve();
        
        EXPECT_EQ(result, l_False);
        EXPECT_EQ(solverAndMock.glucoseMock->mockctrl_getAmountOfInvocations(), 3);
        test_allAddedClausesDeactivated(*solverAndMock.glucoseMock);
    }
    
    TEST(RSARARSolver, DISABLED_terminatesOnSAT) {
        auto solverAndMock = createDefaultTestSetup();
        
        solverAndMock.glucoseMock->mockctrl_setDefaultSolveResult(false);
        solverAndMock.glucoseMock->mockctrl_setResultInInvocation(1, true);
        
        solverAndMock.arSolver->init(*solverAndMock.minProblem);
        lbool result = solverAndMock.arSolver->solve();
        
        EXPECT_EQ(result, l_True);
        EXPECT_EQ(solverAndMock.glucoseMock->mockctrl_getAmountOfInvocations(), 2);
    }
    
    TEST(RSARARSolver, addedApproximationClausesRepresentEquivalencies) {
        auto solverAndMock = createDefaultTestSetup();
        solverAndMock.glucoseMock->mockctrl_setDefaultSolveResult(false);
        
        solverAndMock.glucoseMock->mockctrl_callOnSolve([&solverAndMock](int round) {
            auto checker = createEquivalencyChecker();
            
            checker->addClauses(solverAndMock.glucoseMock->mockctrl_getAddedClauses());
            
            if (round == 0) {
                // see createDefaultTestSetup() for initial conjectures and mock heuristic
                // configuration
                EXPECT_TRUE(checker->isAllEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                    {Lit(2, 1),
                                                        Lit(3, 0),
                                                        Lit(0, 1)}));
                EXPECT_TRUE(checker->isAllEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                    {Lit(4, 0),
                                                        Lit(5, 1)}));
                
                // var. 1 should be removed before the first call to solve
                EXPECT_FALSE(checker->isEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                 Lit(2, 1),
                                                 Lit(1, 0)));
                
                // the two "equivalency groups" should not be equivalent
                EXPECT_FALSE(checker->isEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                  Lit(2, 1),
                                                  Lit(4, 0)));
            }
            else if (round == 1) {
                EXPECT_TRUE(checker->isAllEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                    {Lit(2, 1),
                                                        Lit(3, 0),
                                                        Lit(0, 1)}));

                
                // var. 1 should be removed before the first call to solve
                EXPECT_FALSE(checker->isEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                  Lit(2, 1),
                                                  Lit(1, 0)));
                
                // the two "equivalency groups" should not be equivalent
                EXPECT_FALSE(checker->isEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                  Lit(2, 1),
                                                  Lit(4, 0)));
                
                // the smaller "equivalency group" should not exist anymore
                EXPECT_FALSE(checker->isAllEquivalent(solverAndMock.glucoseMock->mockctrl_getLastAssumptionLits(),
                                                    {Lit(4, 0),
                                                        Lit(5, 1)}));
            }
            else if (round == 3) {
                // everything should be deactivated by now
                test_allAddedClausesDeactivated(*(solverAndMock.glucoseMock));
            }
        });
        
        solverAndMock.arSolver->init(*solverAndMock.minProblem);
        solverAndMock.arSolver->solve();
    }
    
    
    static void test_acceptanceTest(std::string filename,
                                    bool expectedResult,
                                    bool addHeuristics = true) {
        CNFProblem problem;
        problem.readDimacsFromFile(filename.c_str());
        ASSERT_FALSE(problem.nClauses() == 0) << "Could not read test problem file.";
        
        // Create random conjectures and a random deactivation sequence, "faking" the
        // results of random simulation. The resulting approximations are almost certainly unsatisfiable.
        // This means working with particularly bad conjectures, which is useful for testing and okay
        // since random simulation is not tested here.
        auto randomLiterals = pickLiterals(problem, 10);
        auto randomConjectures = createRandomConjectures(randomLiterals, problem);
        auto randomHeuristic = createRandomHeuristic(randomLiterals);
                
        auto solver = new ARSolver(std::move(randomConjectures), std::move(randomHeuristic));
        solver->init(problem);
        auto result = solver->solve();
        EXPECT_EQ(result, lbool(expectedResult));
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_1) {
        return test_acceptanceTest(std::string("problems/flat200-6.cnf"), true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_3) {
        return test_acceptanceTest(std::string("problems/flat200-6.cnf"), true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_6) {
        return test_acceptanceTest(std::string("problems/flat200-6.cnf"), true);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_1_onlyInternalHeur) {
        return test_acceptanceTest(std::string("problems/flat200-1.cnf"), true, false);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_3_onlyInternalHeur) {
        return test_acceptanceTest(std::string("problems/flat200-3.cnf"), true, false);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_flat200_6_onlyInternalHeur) {
        return test_acceptanceTest(std::string("problems/flat200-6.cnf"), true, false);
    }
    
    TEST(RSARARSolver, acceptanceTest_problem_6s33) {
        test_acceptanceTest(std::string("problems/6s33.cnf"), false);
    }
}


