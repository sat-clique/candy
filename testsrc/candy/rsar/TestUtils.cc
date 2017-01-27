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
#include "TestUtils.h"

#include <simp/SimpSolver.h>

#include <iostream>

namespace Candy {

    EquivalencyChecker::EquivalencyChecker()
    : m_solver(std::unique_ptr<Glucose::SimpSolver>(new Glucose::SimpSolver())), m_maxVar(0) {
        m_solver->setIncrementalMode();
        m_solver->certifiedUNSAT = false;
    }
    
    void EquivalencyChecker::addClauses(const std::vector<Cl>& clauses) {
        for (auto& clause : clauses) {
            for (auto lit : clause) {
                ASSERT_TRUE(Glucose::var(lit) <= m_maxVar);
            }
            m_solver->addClause(clause);
        }
    }
    
    Var EquivalencyChecker::createVariable() {
        m_maxVar = m_solver->newVar();
        return m_maxVar;
    }
    
    void EquivalencyChecker::createVariables(Var max) {
        while (m_maxVar < max) {
            createVariable();
        }
    }
    
    void EquivalencyChecker::finishedAddingRegularVariables() {
        m_solver->initNbInitialVars(m_maxVar+1);
    }
    
    bool EquivalencyChecker::isEquivalent(const std::vector<Lit>& assumptions, Lit a, Lit b) {
        std::vector<Lit> extendedAssumptions {assumptions};
        
        Lit assumption1 = Glucose::mkLit(createVariable(), 1);
        Lit assumption2 = Glucose::mkLit(createVariable(), 1);
        
        addClauses({Cl{assumption1, a}, Cl{assumption1, ~b},
            Cl{assumption2, ~a}, Cl{assumption2, b}});
        
        extendedAssumptions.push_back(~assumption1);
        extendedAssumptions.push_back(assumption2);
        
        if (!solve(extendedAssumptions)) {
            extendedAssumptions[extendedAssumptions.size()-2] = assumption1;
            extendedAssumptions[extendedAssumptions.size()-1] = ~assumption2;
            return !solve(extendedAssumptions);
        }
        else {
            return false;
        }
    }
    
    bool EquivalencyChecker::isAllEquivalent(const std::vector<Lit>& assumptions, const std::vector<Lit>& equivalentLits) {
        assert(equivalentLits.size() > 1);
        
        bool allEquiv = true;
        for (size_t i = 0; i < equivalentLits.size()-1 && allEquiv; ++i) {
            assert(equivalentLits[i] <= m_maxVar && equivalentLits[i+1] <= m_maxVar);
            allEquiv &= isEquivalent(assumptions, equivalentLits[i], equivalentLits[i+1]);
        }
        
        return allEquiv;
    }
    
    bool EquivalencyChecker::isBackbones(const std::vector<Lit>& assumptions, const std::vector<Lit>& backboneLits) {
        bool allBackbone = true;
        for (auto lit : backboneLits) {
            Lit assumption = Glucose::mkLit(createVariable(), 1);
            addClauses({Cl{~lit, assumption}});
            std::vector<Lit> extendedAssumptions {assumptions};
            extendedAssumptions.push_back(~assumption);
            allBackbone &= !solve(extendedAssumptions);
        }
        return allBackbone;
    }
    
    bool EquivalencyChecker::solve(const std::vector<Lit>& assumptions) {
        return m_solver->solve(assumptions, false, true);
    }
    
    
    TEST(RSARTestUtils, EquivalencyChecker_checkEquivalences) {
        EquivalencyChecker checker;
        
        checker.createVariables(3);
        
        checker.finishedAddingRegularVariables();
        
        checker.addClauses({{Glucose::mkLit(0, 0), Glucose::mkLit(1, 1)},
            {Glucose::mkLit(1, 0), Glucose::mkLit(0, 1)},
            {Glucose::mkLit(2, 0), Glucose::mkLit(3, 1)}});
        
        EXPECT_FALSE(checker.isEquivalent({}, Glucose::mkLit(2, 1), Glucose::mkLit(3,1)));
        EXPECT_TRUE(checker.isEquivalent({}, Glucose::mkLit(0, 1), Glucose::mkLit(1,1)));
        EXPECT_FALSE(checker.isEquivalent({}, Glucose::mkLit(0, 0), Glucose::mkLit(1,1)));
    }
    
    TEST(RSARTestUtils, EquivalencyChecker_checkBackbones) {
        EquivalencyChecker checker;
        
        checker.createVariables(2);
        checker.finishedAddingRegularVariables();
        checker.createVariables(3);

        checker.addClauses({
            {Glucose::mkLit(0, 0), Glucose::mkLit(1, 1)},
            {Glucose::mkLit(3, 0), Glucose::mkLit(0, 1)},
            {Glucose::mkLit(2, 0), Glucose::mkLit(3, 1)}});
        
        
        EXPECT_TRUE(checker.isBackbones({Glucose::mkLit(3, 1)}, {Glucose::mkLit(0, 1)}));
        EXPECT_FALSE(checker.isBackbones({Glucose::mkLit(3, 1)}, {Glucose::mkLit(2, 0)}));
    }
}
