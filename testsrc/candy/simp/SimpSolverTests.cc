#include <gtest/gtest.h>
#include <core/SolverTypes.h>
#include <simp/SimpSolver.h>
#include <core/Solver.h>

// This is a regression test provoking a bug due to SimpSolver appending,
// instead of replacing the vector of assumption literals
namespace Candy {
    TEST(SimpSolverTests, basicIncrementalSimpSolver) {
        Glucose::SimpSolver underTest;
        underTest.setIncrementalMode();
        
        Var var1 = underTest.newVar();
        Var var2 = underTest.newVar();
        Var assumptionVar = underTest.newVar();
        
        underTest.initNbInitialVars(2);
        
        // This problem is satisfiable for any assignment of assumptionVar.
        std::vector<Cl> clauses = {Cl{Glucose::mkLit(var1, 0), Glucose::mkLit(var2, 1)},
            Cl{Glucose::mkLit(assumptionVar, 0), Glucose::mkLit(var1, 0)},
            Cl{Glucose::mkLit(assumptionVar, 1), Glucose::mkLit(var1, 1)}};
        for (auto& clause : clauses) {
            underTest.addClause(clause);
        }
                            
        EXPECT_TRUE(underTest.solve(std::vector<Lit>{Glucose::mkLit(assumptionVar, 0)}));
        EXPECT_TRUE(underTest.solve(std::vector<Lit>{Glucose::mkLit(assumptionVar, 1)}));
    }
    
    
    // TODO: this test should be moved into the "core" test module.
    TEST(SimpSolverTests, basicIncrementalSolver) {
        Glucose::Solver underTest;
        underTest.setIncrementalMode();
        
        Var var1 = underTest.newVar();
        Var var2 = underTest.newVar();
        Var assumptionVar = underTest.newVar();
        
        underTest.initNbInitialVars(2);
        
        // This problem is satisfiable for any assignment of assumptionVar.
        std::vector<Cl> clauses = {Cl{Glucose::mkLit(var1, 0), Glucose::mkLit(var2, 1)},
            Cl{Glucose::mkLit(assumptionVar, 0), Glucose::mkLit(var1, 0)},
            Cl{Glucose::mkLit(assumptionVar, 1), Glucose::mkLit(var1, 1)}};
        for (auto& clause : clauses) {
            underTest.addClause(clause);
        }
        
        EXPECT_TRUE(underTest.solve(std::vector<Lit>{Glucose::mkLit(assumptionVar, 0)}));
        EXPECT_TRUE(underTest.solve(std::vector<Lit>{Glucose::mkLit(assumptionVar, 1)}));
    }
}
