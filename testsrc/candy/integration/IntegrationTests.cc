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

    static void acceptanceTest(CandySolverInterface* solver, const char* filename, bool expectedResult, bool install_static_allocator) {
        CNFProblem problem;
        problem.readDimacsFromFile(filename);
        // ASSERT_FALSE(problem.size() == 0) << "Could not read test problem file.";
        solver->init(problem);
        ClauseAllocator* allocator = nullptr;
        if (install_static_allocator) {
            allocator = solver->setupGlobalAllocator();
        }
        auto result = solver->solve();
        GTEST_COUT << filename << std::endl;
        delete solver;
        if (allocator != nullptr) {
            delete allocator;
        }
        EXPECT_EQ(result, lbool(expectedResult));
    }

    static void testAllProblems(bool static_allocator, bool use_ts_pr, bool use_lrb) {
        acceptanceTest(createSolver(use_ts_pr, use_lrb, false), "problems/sat/fuzz01.cnf", true, static_allocator); 
        acceptanceTest(createSolver(use_ts_pr, use_lrb, false), "problems/sat/fuzz02.cnf", true, static_allocator);
        acceptanceTest(createSolver(use_ts_pr, use_lrb, false), "problems/sat/fuzz03.cnf", true, static_allocator);
        acceptanceTest(createSolver(use_ts_pr, use_lrb, false), "problems/sat/fuzz04.cnf", true, static_allocator);
        acceptanceTest(createSolver(use_ts_pr, use_lrb, false), "problems/sat/trivial0.cnf", true, static_allocator);
        acceptanceTest(createSolver(use_ts_pr, use_lrb, false), "problems/sat/trivial2.cnf", true, static_allocator);
        acceptanceTest(createSolver(use_ts_pr, use_lrb, false), "problems/unsat/trivial1.cnf", false, static_allocator);
        acceptanceTest(createSolver(use_ts_pr, use_lrb, false), "problems/unsat/dubois20.cnf", false, static_allocator);
        acceptanceTest(createSolver(use_ts_pr, use_lrb, false), "problems/unsat/hole6.cnf", false, static_allocator);
    }

    TEST(IntegrationTest, test_lrb) {
        testAllProblems(false, false, true);
    }

    TEST(IntegrationTest, test_lrb_with_static_propagate) {
        testAllProblems(false, true, true);
    }

    TEST(IntegrationTest, test_vsids) {
        testAllProblems(false, false, false);
    }

    TEST(IntegrationTest, test_vsids_with_static_propagate) {
        testAllProblems(false, true, false);
    }

    TEST(IntegrationTest, test_lrb_with_static_allocator) {
        testAllProblems(true, false, true);
    }

    TEST(IntegrationTest, test_lrb_with_static_propagate_with_static_allocator) {
        testAllProblems(true, true, true);
    }

    TEST(IntegrationTest, test_vsids_with_static_allocator) {
        testAllProblems(true, false, false);
    }

    TEST(IntegrationTest, test_vsids_with_static_propagate_with_static_allocator) {
        testAllProblems(true, true, false);
    }
    
}
