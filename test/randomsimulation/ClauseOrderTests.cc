#include <gtest/gtest.h>

#include <gates/GateAnalyzer.h>
#include <randomsimulation/ClauseOrder.h>

#include "TestGateStructure.h"

namespace randsim {
    // TODO: move self-tests to own test suite file
    
    bool containsClause(const Glucose::For& formula, const Glucose::Cl& clause);
    
    TEST(RSTestMockGateStructureBuilding, GateStructureBuilderTest_mostSimple) {
        auto gateBuilder = createGateStructureBuilder();
        auto cnf = gateBuilder->build();
        auto& formula = cnf->getProblem();
        
        EXPECT_TRUE(formula.size() == 1);
        EXPECT_TRUE(containsClause(formula, Glucose::Cl({Glucose::mkLit(0, 1)})));
    }
    
    TEST(RSTestMockGateStructureBuilding, GateStructureBuilderTest_andGate) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withAnd({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0,1));
        auto cnf = gateBuilder->build();
        auto& formula = cnf->getProblem();
        
        EXPECT_TRUE(formula.size() == 4);
        
        EXPECT_TRUE(containsClause(formula, Glucose::Cl({Glucose::mkLit(1, 0), Glucose::mkLit(2, 0), Glucose::mkLit(0, 1)})));
        EXPECT_TRUE(containsClause(formula, Glucose::Cl({Glucose::mkLit(1, 1), Glucose::mkLit(0, 0)})));
        EXPECT_TRUE(containsClause(formula, Glucose::Cl({Glucose::mkLit(2, 1), Glucose::mkLit(0, 0)})));
        EXPECT_TRUE(containsClause(formula, Glucose::Cl({Glucose::mkLit(0, 1)})));
    }
    
    TEST(RSTestMockGateStructureBuilding, GateStructureBuilderTest_orGate) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0,1));
        auto cnf = gateBuilder->build();
        auto& formula = cnf->getProblem();
        
        EXPECT_TRUE(formula.size() == 4);
        
        EXPECT_TRUE(containsClause(formula, Glucose::Cl({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1), Glucose::mkLit(0, 0)})));
        EXPECT_TRUE(containsClause(formula, Glucose::Cl({Glucose::mkLit(1, 0), Glucose::mkLit(0, 1)})));
        EXPECT_TRUE(containsClause(formula, Glucose::Cl({Glucose::mkLit(2, 0), Glucose::mkLit(0, 1)})));
        EXPECT_TRUE(containsClause(formula, Glucose::Cl({Glucose::mkLit(0, 1)})));
    }
    
    TEST(RSTestMockGateStructureBuilding, GateAnalyzerTest_simple) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0,1));
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga(*formula);
        ga.analyze();
        EXPECT_EQ(ga.getGateCount(), 1);
    }
    
    TEST(RSTestMockGateStructureBuilding, GateAnalyzerTest_threeGates) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0,1));
        gateBuilder->withAnd({Glucose::mkLit(3, 1), Glucose::mkLit(4, 1), Glucose::mkLit(5, 1)}, Glucose::mkLit(1,1));
        gateBuilder->withAnd({Glucose::mkLit(6, 1), Glucose::mkLit(7, 1)}, Glucose::mkLit(2,1));
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga(*formula);
        ga.analyze();
        EXPECT_EQ(ga.getGateCount(), 3);
    }
    
    bool containsClause(const Glucose::For& formula, const Glucose::Cl& clause) {
        bool found = false;
        for (auto fcl : formula) {
            found |= (*fcl == clause); // TODO: make this independent of literal positions
        }
        return found;
    }
}
