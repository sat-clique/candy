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
#include <candy/core/CNFProblem.h>
#include <candy/core/CandySolverInterface.h>
#include <candy/core/CandySolverResult.h>
#include <candy/frontend/CandyBuilder.h>

#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <string>

#define GTEST_COUT std::cerr << "[ INFO     ] "

namespace Candy {

    /** Test Trail Integrity in Incremental Mode (Thanks to Maximilian Schick) */
    TEST(IntegrationTest, test_mschick) {
        std::string filename = std::string(getenv("DATADIR")) + "/6s33.cnf";
        GTEST_COUT << filename << std::endl;

        CNFProblem problem;
        problem.readDimacsFromFile(filename.c_str());

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

    /** Test Trail Integrity in Incremental Mode (Model Enumeration) */
    TEST(IntegrationTest, test_model_enumeration) {
        std::string filename = std::string(getenv("DATADIR")) + "/sat100.cnf";
        GTEST_COUT << filename << std::endl;

        CNFProblem problem;
        problem.readDimacsFromFile(filename.c_str());
    
        // Solve first time
        SolverOptions::opt_preprocessing = false;
        VariableEliminationOptions::opt_use_elim = false;
        CandySolverInterface* solver = createSolver();
        solver->init(problem);
        lbool result = solver->solve();
        EXPECT_EQ(result, l_True);

        // Solve second time, exclude first model
        std::vector<Lit> model = solver->getCandySolverResult().getModelLiterals();
        std::vector<Lit> clause;
        for (Lit lit : model) clause.push_back(lit);
        solver->init(CNFProblem { clause });
        result = solver->solve();
        EXPECT_EQ(result, l_True);
        std::vector<Lit> prev = model;
        model = solver->getCandySolverResult().getModelLiterals();
        EXPECT_EQ(model.size(), prev.size());
        bool different_model = false;
        for (unsigned int i = 0; i < model.size() && !different_model; i++) {
            EXPECT_EQ(model[i].var(), prev[i].var());
            different_model = (model[i] != prev[i]);
        }
        EXPECT_TRUE(different_model);

        // Count models with assumption 1_L
        solver->getAssignment().setAssumptions({ 1_L });
        unsigned int count = 1;
        while (result == l_True && count < 2000) {
            count++;
            clause.clear();
            for (Lit lit : model) clause.push_back(~lit);
            solver->init(CNFProblem { clause });
            result = solver->solve();
            if (result == l_True) {
                EXPECT_TRUE(std::any_of(clause.begin(), clause.end(), [solver](Lit lit) { return solver->getCandySolverResult().satisfies(lit);}));
                EXPECT_TRUE(solver->getCandySolverResult().satisfies(1_L));
                model = solver->getCandySolverResult().getModelLiterals();
            }
        }
        EXPECT_EQ(count, 1069);
        
        delete solver;
    }
    
}
