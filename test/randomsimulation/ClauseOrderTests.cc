/* Copyright (c) 2017 Felix Kutzner
 
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

#include <core/SolverTypes.h>
#include <core/CNFProblem.h>


#include <gates/GateAnalyzer.h>
#include <randomsimulation/ClauseOrder.h>

#include "TestGateStructure.h"
#include "TestUtils.h"

#include <unordered_set>

namespace randsim {
    
    // TODO: these tests do not cover Plaistedt-Greenbaum-optimized gate encodings yet.
    
    // TODO: test ClauseOrder with gate filter


    // utility functions
    
    static bool equals(std::vector<Glucose::Cl*>& a, const std::vector<const Glucose::Cl*>& b);
    template<typename T> static bool equals(const std::vector<T>& a, const std::vector<T>& b);
    template<typename X, typename Y> static bool contains(X* iterable, Y thing);
    static bool containsClauses(GateAnalyzer& analyzer, Glucose::Lit outputLiteral, const std::vector<const Glucose::Cl*>& clauses);
    static bool allClausesContain(Glucose::Lit literal, const std::vector<const Glucose::Cl*>& clauses);
    static bool appearsOrdered(Glucose::Var firstVar, Glucose::Var secondVar, const std::vector<Glucose::Lit> &literals);
    
    
    
    static void test_noGates(ClauseOrder& underTest) {
        auto gateBuilder = createGateStructureBuilder();
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga(*formula);
        ga.analyze();
        ASSERT_EQ(ga.getGateCount(), 0);
        
        underTest.readGates(ga);
        EXPECT_EQ(underTest.getAmountOfVars(), 0);
        EXPECT_EQ(underTest.getGateOutputsOrdered().size(), 0);
        EXPECT_EQ(underTest.getInputVariables().size(), 0);
    }
    
    TEST(RSClauseOrderTest, noGates_recursiveImpl) {
        auto underTest = createRecursiveClauseOrder();
        test_noGates(*underTest);
    }
    
    
    
    static void test_singleGate(ClauseOrder& underTest) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0,1));
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga(*formula);
        ga.analyze();
        
        ASSERT_EQ(ga.getGateCount(), 1);
        
        underTest.readGates(ga);
        
        EXPECT_EQ(underTest.getAmountOfVars(), 3);
        EXPECT_EQ(underTest.getGateOutputsOrdered().size(), 1);
        EXPECT_EQ(underTest.getInputVariables().size(), 2);
        
        EXPECT_TRUE(equals(underTest.getInputVariables(), std::vector<Glucose::Var>({1,2})));
        
        for (auto lit : underTest.getGateOutputsOrdered()) {
            EXPECT_TRUE(allClausesContain(lit, underTest.getClauses(0)));
            EXPECT_TRUE(containsClauses(ga, lit, underTest.getClauses(0)));
        }
    }
    
    TEST(RSClauseOrderTest, singleGate_recursiveImpl) {
        auto underTest = createRecursiveClauseOrder();
        test_singleGate(*underTest);
    }
    
    
    
    static void test_fewGates(ClauseOrder& underTest) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0,1));
        gateBuilder->withAnd({Glucose::mkLit(3, 1), Glucose::mkLit(4, 1)}, Glucose::mkLit(1,1));
        gateBuilder->withAnd({Glucose::mkLit(5, 1), Glucose::mkLit(4, 1)}, Glucose::mkLit(3,1));
        
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga(*formula);
        ga.analyze();
        
        ASSERT_EQ(ga.getGateCount(), 3);
        
        underTest.readGates(ga);
        
        EXPECT_EQ(underTest.getAmountOfVars(), 6);
        EXPECT_EQ(underTest.getGateOutputsOrdered().size(), 3);
        EXPECT_EQ(underTest.getInputVariables().size(), 3);
        
        EXPECT_TRUE(equals(underTest.getInputVariables(), std::vector<Glucose::Var>({2,5,4})));

        
        EXPECT_TRUE(appearsOrdered(1, 0, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(3, 0, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(3, 1, underTest.getGateOutputsOrdered()));
        
        for (auto lit : underTest.getGateOutputsOrdered()) {
            EXPECT_TRUE(allClausesContain(lit, underTest.getClauses(Glucose::var(lit))));
            EXPECT_TRUE(containsClauses(ga, lit, underTest.getClauses(Glucose::var(lit))));
        }
    }
    
    TEST(RSClauseOrderTest, fewGates_recursiveImpl) {
        auto underTest = createRecursiveClauseOrder();
        test_fewGates(*underTest);
    }
    
    
    
    static void test_manyGates(ClauseOrder& underTest) {
        auto gateBuilder = createGateStructureBuilder();
        gateBuilder->withOr({Glucose::mkLit(1, 1), Glucose::mkLit(2, 1)}, Glucose::mkLit(0,1));
        gateBuilder->withAnd({Glucose::mkLit(3, 1), Glucose::mkLit(4, 1)}, Glucose::mkLit(1,1));
        
        gateBuilder->withAnd({Glucose::mkLit(5, 1), Glucose::mkLit(4, 1), Glucose::mkLit(6, 1)}, Glucose::mkLit(3,0));
        gateBuilder->withAnd({Glucose::mkLit(7, 0), Glucose::mkLit(6, 0)}, Glucose::mkLit(4,1));
        
        
        gateBuilder->withOr({Glucose::mkLit(8, 1), Glucose::mkLit(9,0), Glucose::mkLit(5,0)}, Glucose::mkLit(7,0));
        gateBuilder->withAnd({Glucose::mkLit(10, 1), Glucose::mkLit(9,0), Glucose::mkLit(6,0)}, Glucose::mkLit(8,0));
        gateBuilder->withAnd({Glucose::mkLit(11, 1), Glucose::mkLit(12,0), Glucose::mkLit(13,0)}, Glucose::mkLit(5,0));
        

        
        auto formula = gateBuilder->build();
        
        GateAnalyzer ga(*formula, 0, true, false, false, false);
        ga.analyze();
        
        ASSERT_EQ(ga.getGateCount(), 7);
        ASSERT_EQ(ga.getRoots().size(), 1);
        ASSERT_EQ(ga.getRoots()[0]->size(), 1);
        
        underTest.readGates(ga);
        
        EXPECT_EQ(underTest.getAmountOfVars(), 14);
        EXPECT_EQ(underTest.getGateOutputsOrdered().size(), 7);
        EXPECT_EQ(underTest.getInputVariables().size(), 7);
        
        EXPECT_TRUE(equals(underTest.getInputVariables(), std::vector<Glucose::Var>({2,6,9,10,11,12,13})));
        
        
        EXPECT_TRUE(appearsOrdered(1, 0, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(3, 0, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(3, 1, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(4, 3, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(7, 4, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(8, 7, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(5, 3, underTest.getGateOutputsOrdered()));
        EXPECT_TRUE(appearsOrdered(5, 7, underTest.getGateOutputsOrdered()));
        
        for (auto lit : underTest.getGateOutputsOrdered()) {
            EXPECT_TRUE(allClausesContain(lit, underTest.getClauses(Glucose::var(lit))));
            EXPECT_TRUE(containsClauses(ga, lit, underTest.getClauses(Glucose::var(lit))));
        }
    }
    
    TEST(RSClauseOrderTest, manyGates_recursiveImpl) {
        auto underTest = createRecursiveClauseOrder();
        test_manyGates(*underTest);
    }

    
    
    
    
    
    static bool equals(std::vector<Glucose::Cl*>& a, const std::vector<const Glucose::Cl*>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        
        std::unordered_set<const Glucose::Cl*> set_a;
        std::unordered_set<const Glucose::Cl*> set_b;
        
        set_a.insert(a.begin(), a.end());
        set_b.insert(b.begin(), b.end());
        
        return set_a == set_b;
    }
    
    template<typename T>
    static bool equals(const std::vector<T>& a, const std::vector<T>& b) {
        if (a.size() != b.size()) {
            return false;
        }
        
        std::unordered_set<T> set_a;
        std::unordered_set<T> set_b;
        
        set_a.insert(a.begin(), a.end());
        set_b.insert(b.begin(), b.end());
        
        return set_a == set_b;
    }
    
    template<typename X, typename Y>
    static bool contains(X* iterable, Y thing) {
        return std::find(iterable->begin(), iterable->end(), thing) != iterable->end();
    }
    
    static bool containsClauses(GateAnalyzer& analyzer, Glucose::Lit outputLiteral, const std::vector<const Glucose::Cl*>& clauses) {
        auto &gate = analyzer.getGate(outputLiteral);
        return gate.isDefined() && (equals(gate.getForwardClauses(), clauses) || equals(gate.getBackwardClauses(), clauses));
    }
    
    static bool allClausesContain(Glucose::Lit literal, const std::vector<const Glucose::Cl*>& clauses) {
        for (auto clause : clauses) {
            if (!contains(clause, literal)) {
                return false;
            }
        }
        return true;
    }
    
    static bool appearsOrdered(Glucose::Var firstVar, Glucose::Var secondVar, const std::vector<Glucose::Lit> &literals) {
        bool foundFirstLit = false;
        bool foundSecondLit = false;
        for (auto lit : literals) {
            foundFirstLit |= Glucose::var(lit) == firstVar;
            foundSecondLit |= Glucose::var(lit) == secondVar;
            if (foundSecondLit && !foundFirstLit) {
                return false;
            }
        }
        return foundFirstLit && foundSecondLit;
    }
}
