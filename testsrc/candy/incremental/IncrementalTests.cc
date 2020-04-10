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

    /** Test Trail Integrity in Incremental Mode (Thanks to Maximilian Schick) */
    TEST(IntegrationTest, test_mschick) {
        GTEST_COUT << "problems/6s33.cnf" << std::endl;

        CNFProblem problem;
        problem.readDimacsFromFile("problems/6s33.cnf");

        Cl assumptions1 = { 1837_L, ~1939_L, ~426_L, ~557_L, ~661_L };
        Cl assumptions2 = { 1837_L, ~1939_L, ~426_L, ~557_L, 661_L };
    
        CandySolverInterface* solver = createSolver();
        solver->init(problem);

        solver->getAssignment().setAssumptions(assumptions1);
        lbool result = solver->solve();
        EXPECT_EQ(result, l_False);

        solver->getAssignment().setAssumptions(assumptions2);
        result = solver->solve();
        EXPECT_EQ(result, l_False);
        
        delete solver;
    }
    
}
