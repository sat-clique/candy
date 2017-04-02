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


#include "SolverAdapter.h"

#include <simp/SimpSolver.h>
#include <utils/MemUtils.h>

namespace Candy {
    SolverAdapter::SolverAdapter() noexcept {
        
    }
    
    SolverAdapter::~SolverAdapter() {
        
    }
    
    class GlucoseAdapter : public SolverAdapter {
    public:
        bool solve(const std::vector<Lit> &assumptions, bool doSimp, bool turnOffSimp) override {
            if (doSimp) {
                m_solver.eliminate(turnOffSimp);
                if (!m_solver.okay()) {
                    return false;
                }
            }
            m_solver.disablePreprocessing();
            return l_True == m_solver.solve(assumptions);
        }
        
        bool solve() override {
            return l_True == m_solver.solve({});
        }
        
        bool addClause(const Cl &clause) override {
            return m_solver.addClause(clause);
        }
        
        void insertClauses(const CNFProblem &problem) override {
            m_solver.addClauses(problem);
        }
        
        void setFrozen(Var variable, bool frozen) override {
            m_solver.setFrozen(variable, frozen);
        }
        
        bool simplify() override {
            m_solver.setPropBudget(1);
            m_solver.solve({});
            m_solver.budgetOff();
            return true;
        }
        
        bool isEliminated(Var var) override {
            return m_solver.isEliminated(var);
        }
        
        void setIncrementalMode() override {
            m_solver.setIncrementalMode();
        }
        
        void initNbInitialVars(int n) override {
            m_solver.initNbInitialVars(n);
        }

        void setParsing(bool parsing) override {
	  // TODO: m_solver.parsing = parsing;
        }
        
        const std::vector<Lit>& getConflict() override {
            return m_solver.getConflict();
        }
        
        Var newVar() override {
            return m_solver.newVar();
        }
        
        int getNVars() const override {
            return m_solver.nVars();
        }
        
        /*const Glucose::SimpSolver& getSolver() override {
            return m_solver;
        }*/
        
        virtual ~GlucoseAdapter() {
            
        }
        
        
        GlucoseAdapter()
        : SolverAdapter(),
        m_ownedSolver(backported_std::make_unique<SimpSolver<>>()),
        m_solver(*m_ownedSolver) {
        }
        
        explicit GlucoseAdapter(SimpSolver<>& solver)
        : SolverAdapter(),
        m_ownedSolver(nullptr),
        m_solver(solver) {
        }
        
        GlucoseAdapter(const GlucoseAdapter& other) = delete;
        GlucoseAdapter& operator= (const GlucoseAdapter& other) = delete;
        
    private:
        std::unique_ptr<SimpSolver<>> m_ownedSolver;
        SimpSolver<>& m_solver;
    };
    
    std::unique_ptr<SolverAdapter> createGlucoseAdapter() {
        return backported_std::make_unique<GlucoseAdapter>();
    }
    
    std::unique_ptr<SolverAdapter> createNonowningGlucoseAdapter(SimpSolver<>& solver) {
        return backported_std::make_unique<GlucoseAdapter>(solver);
    }

}
