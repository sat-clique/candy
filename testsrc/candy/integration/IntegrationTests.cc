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
#include <candy/utils/CandyBuilder.h>

extern "C" {
#include <candy/ipasir/ipasir.h>
}

#include <iostream>
#include <algorithm>

#define GTEST_COUT std::cerr << "[ INFO     ] "

namespace Candy {

    static const char* CERT = "cert.drat";

    static void acceptanceTest(const char* filename, bool install_static_allocator) {
        GTEST_COUT << filename << std::endl;
        CNFProblem problem;
        problem.readDimacsFromFile(filename);
        CandySolverInterface* solver = createSolver(problem);
        ClauseAllocator* allocator = nullptr;
        if (install_static_allocator) {
            GTEST_COUT << "Setup Static Allocator" << std::endl;
            allocator = solver->getClauseDatabase().createGlobalClauseAllocator();
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

    static void testTrivialProblems(bool static_allocator) {
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/trivial0.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/trivial1.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/trivial2.cnf", static_allocator);
    }

    static void testFuzzProblems(bool static_allocator) {
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/fuzz01.cnf", static_allocator); 
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/fuzz02.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/fuzz03.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/fuzz04.cnf", static_allocator);
    }

    static void testRealProblems(bool static_allocator) {
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/dubois20.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/hole6.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/ais6.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/uf75-042.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/uuf75-042.cnf", static_allocator);
    }

    static void testFixedBugs(bool static_allocator) {
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/dd1.cnf", static_allocator);
        SolverOptions::opt_certified_file = CERT;
        acceptanceTest("cnf/dd2.cnf", static_allocator);
    }

    TEST(IntegrationTest, test_vsids) {
        SolverOptions::opt_use_lrb = false;
        ParallelOptions::opt_static_propagate = false;
        ParallelOptions::opt_lb_propagate = false;
        ParallelOptions::opt_3full_propagate = false;
        ParallelOptions::opt_Xfull_propagate = 2;
        testTrivialProblems(false);
        testFuzzProblems(false);
        testRealProblems(false);
        testFixedBugs(false);
    }

    TEST(IntegrationTest, test_vsids_with_static_propagate) {
        SolverOptions::opt_use_lrb = false;
        ParallelOptions::opt_static_propagate = true;
        ParallelOptions::opt_lb_propagate = false;
        ParallelOptions::opt_3full_propagate = false;
        ParallelOptions::opt_Xfull_propagate = 2;
        testFuzzProblems(false);
    }

    TEST(IntegrationTest, test_vsids_with_lb_propagate) {
        SolverOptions::opt_use_lrb = false;
        ParallelOptions::opt_static_propagate = false;
        ParallelOptions::opt_lb_propagate = true;
        ParallelOptions::opt_3full_propagate = false;
        ParallelOptions::opt_Xfull_propagate = 2;
        testTrivialProblems(false);
        testFuzzProblems(false);
        testRealProblems(false);
    }

    TEST(IntegrationTest, test_vsids_with_3full_propagate) {
        SolverOptions::opt_use_lrb = false;
        ParallelOptions::opt_static_propagate = false;
        ParallelOptions::opt_lb_propagate = false;
        ParallelOptions::opt_3full_propagate = true;
        ParallelOptions::opt_Xfull_propagate = 2;
        testTrivialProblems(false);
        testFuzzProblems(false);
        testRealProblems(false);
    }

    TEST(IntegrationTest, test_vsids_with_Xfull_propagate) {
        SolverOptions::opt_use_lrb = false;
        ParallelOptions::opt_static_propagate = false;
        ParallelOptions::opt_lb_propagate = false;
        ParallelOptions::opt_3full_propagate = false;
        ParallelOptions::opt_Xfull_propagate = 3;
        testTrivialProblems(false);
        testFuzzProblems(false);
        testRealProblems(false);
        ParallelOptions::opt_Xfull_propagate = 4;
        testTrivialProblems(false);
        testFuzzProblems(false);
        testRealProblems(false);
        ParallelOptions::opt_Xfull_propagate = 5;
        testTrivialProblems(false);
        testFuzzProblems(false);
        testRealProblems(false);
    }

    TEST(IntegrationTest, test_vsids_with_static_allocator) {
        SolverOptions::opt_use_lrb = false;
        ParallelOptions::opt_static_propagate = false;
        ParallelOptions::opt_lb_propagate = false;
        ParallelOptions::opt_3full_propagate = false;
        ParallelOptions::opt_Xfull_propagate = 2;
        testFuzzProblems(true);
    }

    TEST(IntegrationTest, test_lrb) {
        SolverOptions::opt_use_lrb = true;
        ParallelOptions::opt_static_propagate = false;
        ParallelOptions::opt_lb_propagate = false;
        ParallelOptions::opt_3full_propagate = false;
        ParallelOptions::opt_Xfull_propagate = 2;
        testFuzzProblems(false);
        testRealProblems(false);
    }
    
}
