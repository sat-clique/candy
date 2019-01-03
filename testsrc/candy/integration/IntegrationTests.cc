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

    CandySolverInterface* getSolver(bool use_lrb, bool use_ts_pr) {
        CandyBuilder<> builder { new ClauseDatabase(), new Trail() };
        
        if (use_lrb) {
            if (use_ts_pr) {
                return builder.branchWithLRB().propagateStaticClauses().build();
            } else {
                return builder.branchWithLRB().build();
            }
        } 
        else {
            if (use_ts_pr) {
                return builder.propagateStaticClauses().build();
            } else {
                return builder.build();
            }
        }
    }

    static void acceptanceTest(CandySolverInterface* solver, const char* filename, bool expectedResult) {
        CNFProblem problem;
        problem.readDimacsFromFile(filename);
        // ASSERT_FALSE(problem.getProblem().empty()) << "Could not read test problem file.";
        solver->addClauses(problem);
        auto result = solver->solve();
        GTEST_COUT << filename << std::endl;
        EXPECT_EQ(result, lbool(expectedResult));
    }

    static void testAllProblems(bool use_lrb, bool use_ts_pr) {
        acceptanceTest(getSolver(use_lrb, use_ts_pr), "problems/sat/fuzz01.cnf", true);
        acceptanceTest(getSolver(use_lrb, use_ts_pr), "problems/sat/fuzz02.cnf", true);
        acceptanceTest(getSolver(use_lrb, use_ts_pr), "problems/sat/fuzz03.cnf", true);
        acceptanceTest(getSolver(use_lrb, use_ts_pr), "problems/sat/fuzz04.cnf", true);
        acceptanceTest(getSolver(use_lrb, use_ts_pr), "problems/sat/trivial0.cnf", true);
        acceptanceTest(getSolver(use_lrb, use_ts_pr), "problems/sat/trivial2.cnf", true);
        acceptanceTest(getSolver(use_lrb, use_ts_pr), "problems/unsat/trivial1.cnf", false);
        acceptanceTest(getSolver(use_lrb, use_ts_pr), "problems/unsat/dubois20.cnf", false);
        acceptanceTest(getSolver(use_lrb, use_ts_pr), "problems/unsat/hole6.cnf", false);
    }

    TEST(IntegrationTest, test_lrb) {
        testAllProblems(true, false);
    }

    TEST(IntegrationTest, test_lrb_with_static_propagate) {
        testAllProblems(true, true);
    }

    TEST(IntegrationTest, test_vsids) {
        testAllProblems(false, false);
    }

    TEST(IntegrationTest, test_vsids_with_static_propagate) {
        testAllProblems(false, true);
    }
    
}
