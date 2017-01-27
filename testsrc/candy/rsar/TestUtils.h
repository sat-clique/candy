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

#ifndef X_3D4F7FAE_8852_4861_A031_1808523E1215_TESTUTILS_H
#define X_3D4F7FAE_8852_4861_A031_1808523E1215_TESTUTILS_H

#include <core/SolverTypes.h>
#include <simp/SimpSolver.h>

#include <vector>
#include <memory>


namespace Candy {
    class EquivalencyChecker {
    public:
        EquivalencyChecker();
        void addClauses(const std::vector<Cl>& clauses);
        Var createVariable();
        
        void createVariables(Var max);
        
        void finishedAddingRegularVariables();
        
        bool isEquivalent(const std::vector<Lit>& assumptions, Lit a, Lit b);
        bool isAllEquivalent(const std::vector<Lit>& assumptions, const std::vector<Lit>& equivalentLits);
        bool isBackbones(const std::vector<Lit>& assumptions, const std::vector<Lit>& backboneLits);
        
    private:
        bool solve(const std::vector<Lit>& assumptions);
        
        std::unique_ptr<Glucose::SimpSolver> m_solver;
        Var m_maxVar;
    };
}

#endif
