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

#ifndef X_C53F2B2A_C7F9_4DFF_890B_196A6135FD51_SOLVERADAPTER_H
#define X_C53F2B2A_C7F9_4DFF_890B_196A6135FD51_SOLVERADAPTER_H

#include <core/SolverTypes.h>
#include <core/CNFProblem.h>

#include <vector>
#include <memory>

namespace Candy {
    /**
     * \class SolverAdapter
     *
     * \ingroup RS_AbstractionRefinement
     *
     * \brief An adapter interface for Glucose-like solvers
     *
     * SolverAdapter is an adapter interface for Glucose-like solvers. The
     * intent of its creation is to increase the testability of ARSolver, as
     * Glucose itself cannot readily be mocked or faked. Therefore, this
     * subset of Glucose's interface is somewhat tailored to the ARSolver
     * implementation.
     */
    class SolverAdapter {
    public:
        virtual bool solve(const std::vector<Lit> &assumptions, bool doSimp, bool turnOffSimp) = 0;
        virtual bool solve() = 0;
        virtual bool addClause(const Cl &clause) = 0;
        virtual void insertClauses(const CNFProblem &problem) = 0;
        virtual void setFrozen(Var variable, bool frozen) = 0;
        virtual bool simplify() = 0;
        virtual bool isEliminated(Var var) = 0;
        
        virtual void setIncrementalMode() = 0;
        virtual void initNbInitialVars(int n) = 0;
        
        virtual void setCertifiedUNSAT(bool cu) = 0;
        virtual void setParsing(bool parsing) = 0;
        virtual int getNVars() const = 0;
        
        virtual const std::vector<Lit>& getConflict() = 0;
        virtual Var newVar() = 0;
        
        SolverAdapter() noexcept;
        virtual ~SolverAdapter();
        SolverAdapter(const SolverAdapter &other) = delete;
        SolverAdapter& operator= (const SolverAdapter& other) = delete;
    };
    
    std::unique_ptr<SolverAdapter> createGlucoseAdapter();
}

#endif
