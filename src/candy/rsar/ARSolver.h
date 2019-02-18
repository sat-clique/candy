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

#ifndef X_27915A76_7E30_4C34_8C0C_FD65894E0A42_ARSOLVER_H
#define X_27915A76_7E30_4C34_8C0C_FD65894E0A42_ARSOLVER_H

#include <candy/core/SolverTypes.h>
#include <candy/core/CNFProblem.h>
#include <candy/core/CandySolverInterface.h>
#include "candy/rsar/Heuristics.h"
#include "candy/rsar/Refinement.h"
#include "candy/randomsimulation/Conjectures.h"
#include <unordered_map>
#include <unordered_set>

#include <vector>
#include <memory>

namespace Candy {
    class Conjectures;
    class RefinementHeuristic;
    
    /**
     * \class ARSolver
     *
     * \ingroup RS_AbstractionRefinement
     *
     * \brief A SAT solver performing abstraction refinement.
     *
     * TODO: detailed description
     */
    class ARSolver : public CandySolverInterface {
    public:

        BranchingDiversificationInterface* getBranchingUnit() override {
            return m_solver->getBranchingUnit();
        }

        ClauseAllocator* setupGlobalAllocator() override {
            return m_solver->setupGlobalAllocator();
        }

        virtual Var createVariable() {
        	return maxVars++;
        }

        virtual void init(const CNFProblem& problem, ClauseAllocator* allocator = nullptr) override {
            maxVars = problem.nVars();
        	m_solver->init(problem, allocator);
        }

        virtual void setLearntCallback(void* state, int max_length, void (*learntCallback)(void* state, int* clause)) override { 
            m_solver->setLearntCallback(state, max_length, learntCallback);
        }

        virtual void setTermCallback(void* state, int (*termCallback)(void*)) override {
            m_solver->setTermCallback(state, termCallback);
        }
        
        virtual void setFrozen(Var v, bool freeze) override {
        	m_solver->setFrozen(v, freeze);
        }

    	virtual void setAssumptions(const std::vector<Lit>& assumptions) override {
    		m_solver->setAssumptions(assumptions);
    	}

    	virtual lbool solve() override;

    	virtual void setConfBudget(uint64_t x) override {
    		m_solver->setConfBudget(x);
    	}
    	virtual void setPropBudget(uint64_t x) override {
    		m_solver->setPropBudget(x);
    	}
    	virtual void setInterrupt(bool value) override {
    		m_solver->setInterrupt(value);
    	}
    	virtual void budgetOff() override {
    		m_solver->budgetOff();
    	}

    	// The value of a variable in the last model. The last call to solve must have been satisfiable.
    	virtual lbool modelValue(Var x) const override {
    		return m_solver->modelValue(x);
    	}
    	virtual lbool modelValue(Lit p) const override {
    		return m_solver->modelValue(p);
    	}
        virtual Cl getModel() override {
            return m_solver->getModel();
        }
    	virtual std::vector<Lit>& getConflict() override {
    		return m_solver->getConflict();
    	}

    	virtual Statistics& getStatistics() override {
    		return m_solver->getStatistics();
    	}

    	virtual ClauseDatabase& getClauseDatabase() override {
    		return m_solver->getClauseDatabase();
    	}

    	virtual Trail& getAssignment() override {
    		return m_solver->getAssignment();
    	}

        ARSolver(std::unique_ptr<Conjectures> conjectures, 
            std::unique_ptr<RefinementHeuristic> heuristics, 
            CandySolverInterface* solver = nullptr);

        ARSolver(std::unique_ptr<Conjectures> conjectures, 
            CandySolverInterface* solver = nullptr);

        virtual ~ARSolver();
        ARSolver(const ARSolver& other) = delete;
        ARSolver& operator=(const ARSolver& other) = delete;

    private:
        /**
         * Initializes the abstraction refinement system and performs
         * initialization-time simplification.
         */
        void initialize();

        /** Removes eliminated variables from the conjectures. */
        void reduceConjectures();

        /** Adds the new clauses contained in the approximation delta. */
        void addApproximationClauses(EncodedApproximationDelta& delta);

        int maxVars;

        /**
         * Creates a function returning the set of literals contained in clauses
         * activated by the assumption literals whose activeness provoked
         * unsatisfiability at the previous SAT solver invocation's final conflict.
         */
        std::function<std::unique_ptr<std::unordered_set<Var>>()> createConflictGetter();

        std::unique_ptr<Conjectures> m_conjectures;
        CandySolverInterface* m_solver;
        int m_maxRefinementSteps;
        std::vector<std::unique_ptr<RefinementHeuristic>> m_heuristics;

        std::unique_ptr<RefinementStrategy> m_refinementStrategy;

        /**
         * Problem literals by assumption literal. For backbone encodings, the
         * literals of the rsp. pair are equal.
         */
        std::unordered_map<Lit, std::pair<Lit, Lit>> m_approxLitsByAssumption;
    };

}

#endif
