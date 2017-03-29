#include <gtest/gtest.h>
#include <core/SolverTypes.h>
#include <simp/SimpSolver.h>
#include <core/Solver.h>

// This is a regression test provoking a bug due to SimpSolver appending,
// instead of replacing the vector of assumption literals
namespace Candy {
    TEST(SimpSolverTests, basicIncrementalSimpSolver) {
        SimpSolver underTest;
        underTest.setIncrementalMode();
        
        Var var1 = underTest.newVar();
        Var var2 = underTest.newVar();
        Var assumptionVar = underTest.newVar();
        
        underTest.initNbInitialVars(2);
        
        // This problem is satisfiable for any assignment of assumptionVar.
        std::vector<Cl> clauses = {Cl{mkLit(var1, 0), mkLit(var2, 1)},
            Cl{mkLit(assumptionVar, 0), mkLit(var1, 0)},
            Cl{mkLit(assumptionVar, 1), mkLit(var1, 1)}};
        for (auto& clause : clauses) {
            underTest.addClause(clause);
        }
                            
        EXPECT_TRUE(underTest.solve(std::vector<Lit>{mkLit(assumptionVar, 0)}));
        EXPECT_TRUE(underTest.solve(std::vector<Lit>{mkLit(assumptionVar, 1)}));
    }
    
    
    // TODO: this test should be moved into the "core" test module.
    TEST(SimpSolverTests, basicIncrementalSolver) {
        Solver underTest;
        underTest.setIncrementalMode();
        
        Var var1 = underTest.newVar();
        Var var2 = underTest.newVar();
        Var assumptionVar = underTest.newVar();
        
        underTest.initNbInitialVars(2);
        
        // This problem is satisfiable for any assignment of assumptionVar.
        std::vector<Cl> clauses = {Cl{mkLit(var1, 0), mkLit(var2, 1)},
            Cl{mkLit(assumptionVar, 0), mkLit(var1, 0)},
            Cl{mkLit(assumptionVar, 1), mkLit(var1, 1)}};
        for (auto& clause : clauses) {
            underTest.addClause(clause);
        }
        
        EXPECT_TRUE(l_True == underTest.solve(std::vector<Lit>{mkLit(assumptionVar, 0)}));
        EXPECT_TRUE(l_True == underTest.solve(std::vector<Lit>{mkLit(assumptionVar, 1)}));
    }
}
