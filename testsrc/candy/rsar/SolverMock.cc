/* Copyright (c) 2017 Felix Kutzner (github.com/fkutzner)
 
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

#include "SolverMock.h"
#include "candy/rsar/ARSolver.h"
#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>

namespace Candy {
    
    lbool SolverMock::solve() {
        // TODO: clear assumption lits if appropriate
        m_nClausesAddedSinceLastSolve = 0;
        bool result = m_defaultSolveResult;
        if (m_solveResultInInvocationN.find(m_nInvocations) != m_solveResultInInvocationN.end()) {
            result = m_solveResultInInvocationN[m_nInvocations];
        }
        
        m_eventLog.push_back(SolverMockEvent::SOLVE);
        
        if (m_callOnSolve) {
            m_callOnSolve(m_nInvocations);
        }
        
        ++m_nInvocations;
        
        return lbool(result);
    }
    
    void SolverMock::init(const CNFProblem &problem, ClauseAllocator* allocator, bool lemma) {
        // TODO: fix the unscrupolously non-const CNFProblem interface to update
        // m_nClausesAddedSinceLastSolve
        m_maxCreatedVar = problem.nVars();
        for (const Cl* clause : problem) {
            EXPECT_TRUE(std::all_of(clause->begin(), clause->end(),
                                    [this](Lit l) { return l.var() <= m_maxCreatedVar; }));
            m_addedClauses.push_back(*clause);
            ++m_nClausesAddedSinceLastSolve;
        }
        m_eventLog.push_back(SolverMockEvent::ADD_PROBLEM);
    }
    
    unsigned int SolverMock::nVars() const {
        return m_maxCreatedVar+1;
    }
    
    unsigned int SolverMock::nClauses() const {
        return m_addedClauses.size();
    }
    
    unsigned int SolverMock::nConflicts() const {
        return 0;
    }
    
    void SolverMock::mockctrl_setConflictLits(const std::vector<Lit> & conflictLits) {
        m_conflictLits = conflictLits;
    }
    
    bool SolverMock::mockctrl_isParsingSet() const noexcept {
        return m_isParsingSet;
    }
    
    void SolverMock::mockctrl_setResultInInvocation(int n, bool result) {
        m_solveResultInInvocationN[n] = result;
    }
    
    void SolverMock::mockctrl_setDefaultSolveResult(bool result) noexcept {
        m_defaultSolveResult = result;
    }
    
    int SolverMock::mockctrl_getAmountOfInvocations() const noexcept {
        return m_nInvocations;
    }
    
    const std::vector<SolverMockEvent>& SolverMock::mockctrl_getEventLog() const noexcept {
        return m_eventLog;
    }
    
    void SolverMock::mockctrl_callOnSolve(std::function<void (int)> func) {
        m_callOnSolve = func;
    }
    
    void SolverMock::mockctrl_callOnSimplify(std::function<void (int)> func) {
        m_callOnSimplify = func;
    }
    
    int SolverMock::mockctrl_getAmountOfClausesAddedSinceLastSolve() const noexcept {
        return m_nClausesAddedSinceLastSolve;
    }
    
    const std::vector<Lit>& SolverMock::mockctrl_getLastAssumptionLits() const noexcept {
        return m_lastAssumptionLits;
    }
    
    
    const std::vector<Cl> &SolverMock::mockctrl_getAddedClauses() const noexcept{
        return m_addedClauses;
    }

    
    SolverMock::SolverMock() noexcept :
    m_nInvocations(0),
    m_maxCreatedVar(-1),
    m_conflictLits(),
    m_minAssumptionVar(0),
    m_solveResultInInvocationN(),
    m_defaultSolveResult(false),
    m_callOnSolve(),
    m_callOnSimplify(),
    m_nClausesAddedSinceLastSolve(0),
    m_lastAssumptionLits(),
    m_isParsingSet(false),
    m_eventLog() {
        
    }
    
    SolverMock::~SolverMock() {
        
    }
    
}
