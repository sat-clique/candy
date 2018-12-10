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

#include <gtest/gtest.h>
#include "SolverMock.h"

#include <candy/core/SolverTypes.h>
#include <candy/core/CNFProblem.h>

#include <unordered_map>

namespace Candy {
    TEST(RSARTestUtils, SolverMock_eventRecording) {
        SolverMock underTest;
        CNFProblem problem;
        
        auto expected = std::vector<SolverMockEvent>{};
        EXPECT_EQ(underTest.mockctrl_getEventLog(), expected);
        
        Var var1 = underTest.newVar();
        Var var2 = underTest.newVar();
        
        underTest.addClauses(problem);
        underTest.addClause(Cl{mkLit(var1,0)});
        underTest.eliminate();
        underTest.addClause(Cl{mkLit(var2,0)});
        underTest.solve();
        
        auto expected2 = std::vector<SolverMockEvent>{
            SolverMockEvent::ADD_PROBLEM,
            SolverMockEvent::ADD_CLAUSE,
            SolverMockEvent::SIMPLIFY,
            SolverMockEvent::ADD_CLAUSE,
            SolverMockEvent::SOLVE};
        EXPECT_EQ(underTest.mockctrl_getEventLog(), expected2);
    }
    
    TEST(RSARTestUtils, SolverMock_callbackOnSolve) {
        SolverMock underTest;
        int callN = 0;
        int numCallbackInvocs = 0;
        
        underTest.mockctrl_callOnSolve([&numCallbackInvocs, &callN](int n) {
            EXPECT_EQ(n, callN);
            ++numCallbackInvocs;
        });
        
        underTest.solve();
        ++callN;
        underTest.eliminate();
        underTest.solve();
        ++callN;
        underTest.addClause({});
        underTest.solve();
        ++callN;
        
        EXPECT_EQ(numCallbackInvocs, 3);
    }
    
    TEST(RSARTestUtils, SolverMock_callbackOnSimplify) {
        SolverMock underTest;
        int callN = 0;
        int numCallbackInvocs = 0;
        
        underTest.mockctrl_callOnSimplify([&numCallbackInvocs, &callN](int n) {
            EXPECT_EQ(n, callN);
            ++numCallbackInvocs;
        });
        
        underTest.eliminate();
        ++callN;
        underTest.solve();
        underTest.eliminate();
        ++callN;
        
        EXPECT_EQ(numCallbackInvocs, 2);
    }
    
    TEST(RSARTestUtils, SolverMock_solveResultSettingPlain) {
        SolverMock underTest;
        underTest.mockctrl_setDefaultSolveResult(true);
        underTest.mockctrl_setResultInInvocation(0, false);
        underTest.mockctrl_setResultInInvocation(1, false);
        underTest.mockctrl_setResultInInvocation(2, false);
        
        EXPECT_EQ(underTest.solve(), l_False);
        EXPECT_EQ(underTest.solve(), l_False);
        EXPECT_EQ(underTest.solve(), l_False);
        EXPECT_EQ(underTest.solve(), l_True);
        EXPECT_EQ(underTest.solve(), l_True);
    }
    
    TEST(RSARTestUtils, SolverMock_solveResultSettingSimp) {
        SolverMock underTest;
        underTest.mockctrl_setDefaultSolveResult(true);
        underTest.mockctrl_setResultInInvocation(0, false);
        underTest.mockctrl_setResultInInvocation(1, false);
        underTest.mockctrl_setResultInInvocation(2, false);
        
        EXPECT_EQ(underTest.solve(), l_False);
        EXPECT_EQ(underTest.solve(), l_False);
        EXPECT_EQ(underTest.solve(), l_False);
        EXPECT_EQ(underTest.solve(), l_True);
        EXPECT_EQ(underTest.solve(), l_True);
    }
}
