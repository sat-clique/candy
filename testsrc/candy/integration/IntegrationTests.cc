/* Copyright (c) 2017 Markus Iser (github.com/udopia)

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

#include <candy/core/SolverTypes.h>
#include <candy/core/Trail.h>
#include <candy/core/DRATChecker.h>
#include <candy/core/CandySolverInterface.h>
#include <candy/core/clauses/ClauseDatabase.h>
#include <candy/frontend/CandyBuilder.h>

extern "C" {
#include <candy/ipasir/ipasir.h>
}

#include <iostream>
#include <algorithm>

#define GTEST_COUT std::cerr << "[ INFO     ] "

namespace Candy {

    static const char* CERT = "cert.drat";

    static void acceptanceTest(CandySolverInterface* solver, const char* filename, bool install_static_allocator) {
        GTEST_COUT << filename << std::endl;
        CNFProblem problem;
        problem.readDimacsFromFile(filename);
        solver->init(problem);
        ClauseAllocator* allocator = nullptr;
        if (install_static_allocator) {
            GTEST_COUT << "Setup Static Allocator" << std::endl;
            allocator = solver->setupGlobalAllocator();
        }
        auto result = solver->solve();

        if (result == l_True) {
            CandySolverResult& model = solver->getCandySolverResult();
            bool satisfied = problem.checkResult(model);
            ASSERT_TRUE(satisfied);
        }
        else if (result == l_False) {
            SolverOptions::opt_certified_file = "";
            DRATChecker checker(problem);
            bool proved = checker.check_proof(CERT);
            ASSERT_TRUE(proved);
        }

        delete solver;
        if (allocator != nullptr) {
            delete allocator;
        }
    }

    static void testTrivialProblems(bool static_allocator, bool use_ts_pr, bool use_lrb, bool use_vsidsc) {
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/trivial0.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/trivial1.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/trivial2.cnf", static_allocator);
    }

    static void testFuzzProblems(bool static_allocator, bool use_ts_pr, bool use_lrb, bool use_vsidsc) {
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/fuzz01.cnf", static_allocator); 
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/fuzz02.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/fuzz03.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/fuzz04.cnf", static_allocator);
    }

    static void testRealProblems(bool static_allocator, bool use_ts_pr, bool use_lrb, bool use_vsidsc) {
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/dubois20.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/hole6.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/ais6.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/uf75-042.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest(createSolver(use_ts_pr, use_lrb, use_vsidsc), "cnf/uuf75-042.cnf", static_allocator);
    }

    TEST(IntegrationTest, test_vsids) {
        testTrivialProblems(false, false, false, false);
        testFuzzProblems(false, false, false, false);
        testRealProblems(false, false, false, false);
    }

    TEST(IntegrationTest, test_vsids_with_static_propagate) {
        testFuzzProblems(false, true, false, false);
    }

    TEST(IntegrationTest, test_vsids_with_static_allocator) {
        testFuzzProblems(true, false, false, false);
    }

    TEST(IntegrationTest, test_vsidsc) {
        testFuzzProblems(false, false, false, true);
        testRealProblems(false, false, false, false);
    }

    TEST(IntegrationTest, test_lrb) {
        testFuzzProblems(false, false, true, false);
        testRealProblems(false, false, false, false);
    }
    
}
