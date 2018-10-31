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

    enum class SimplificationHandlingMode {
        DISABLE, /// Disable problem simplification entirely
        RESTRICT, /// Simplify the SAT problem after creating the first approximation
        FREEZE, /// like RESTRICT, but freeze the variables occuring involved in the first approximation
        FULL /// Simplify the problem first, then compute the approximations using the remaining variables
    };
    
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

        // from CandySolverInterface:
        virtual void enablePreprocessing() override {
        	m_solver->enablePreprocessing();
        }
        virtual void disablePreprocessing() override {
        	m_solver->disablePreprocessing();
        }

        virtual void resetCertificate(const char* targetFilename) override {
        	return m_solver->resetCertificate(targetFilename);
        }

        virtual Var newVar() override {
        	return m_solver->newVar();
        }

        virtual void addClauses(const CNFProblem& problem) override {
        	m_solver->addClauses(problem);
        }
        virtual bool addClause(const std::vector<Lit>& lits, bool learnt = false) override {
        	return m_solver->addClause(lits, learnt);
        }
        virtual bool addClause(std::initializer_list<Lit> lits, bool learnt = false) override {
        	return m_solver->addClause(lits, learnt);
        }

        virtual void setLearntCallback(void* state, int max_length, void (*learntCallback)(void* state, int* clause)) override { 
            return m_solver->setLearntCallback(state, max_length, learntCallback);
        }


        /** Runs the underlying SAT solver's simplification system. This method may only be called
         * before clauses containing assumptions have been added to the solver. */
        virtual bool simplify() override {
        	return m_solver->simplify(); // remove satisfied clauses
        }

        virtual bool strengthen() override {
        	return m_solver->strengthen(); // remove false literals from clauses
        }
        virtual bool eliminate() override {
        	return m_solver->eliminate();// Perform variable elimination based simplification.
        }
        virtual bool eliminate(bool use_asymm, bool use_elim) override {
        	return m_solver->eliminate(use_asymm, use_elim);  // Perform variable elimination based simplification.
        }
        virtual bool isEliminated(Var v) const override {
        	return m_solver->isEliminated(v);
        }
        virtual void setFrozen(Var v, bool freeze) override {
        	m_solver->setFrozen(v, freeze);
        }

    	virtual lbool solve() override;
    	virtual lbool solve(std::initializer_list<Lit> assumps) override {
    		assert(0 && "unsupported");
    		return l_Undef;
    	}
    	virtual lbool solve(const std::vector<Lit>& assumps) override {
    		assert(0 && "unsupported");
    		return l_Undef;
    	}

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

    	virtual void printDIMACS() override {
    		m_solver->printDIMACS();
    	}

    	// The value of a variable in the last model. The last call to solve must have been satisfiable.
    	virtual lbool modelValue(Var x) const override {
    		return m_solver->modelValue(x);
    	}
    	virtual lbool modelValue(Lit p) const override {
    		return m_solver->modelValue(p);
    	}
        virtual Cl getModel() {
            return m_solver->getModel();
        }

    	// true means solver is in a conflicting state
    	virtual bool isInConflictingState() const override {
    		return m_solver->isInConflictingState();
    	}
    	virtual std::vector<Lit>& getConflict() override {
    		return m_solver->getConflict();
    	}

    	virtual size_t nClauses() const override {
    		return m_solver->nClauses();
    	}
    	virtual size_t nLearnts() const override {
    		return m_solver->nLearnts();
    	}
    	virtual size_t nVars() const override {
    		return m_solver->nVars();
        }
    	virtual size_t nConflicts() const override {
    		return m_solver->nConflicts();
    	}
    	virtual size_t nPropagations() const override {
    		return m_solver->nPropagations();
    	}

    	// Incremental mode
    	virtual void setIncrementalMode() override {
    		m_solver->setIncrementalMode();
        }

    	virtual bool isIncremental() override {
    		return m_solver->isIncremental();
    	}

    	ARSolver();

        ARSolver(std::unique_ptr<Conjectures> conjectures,
        			 CandySolverInterface* solver,
                     int maxRefinementSteps,
                     std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> heuristics,
                     SimplificationHandlingMode simpHandlingMode);

        virtual ~ARSolver();
        ARSolver(const ARSolver& other) = delete;
        ARSolver& operator=(const ARSolver& other) = delete;

    private:
        /**
         * Initializes the abstraction refinement system and performs
         * initialization-time simplification.
         */
        void init();

        /** Removes eliminated variables from the conjectures. */
        void reduceConjectures();

        /**
         * Adds the new clauses contained in the approximation delta, performing
         * simplification as needed.
         */
        void addInitialApproximationClauses(EncodedApproximationDelta& delta);

        /** Adds the new clauses contained in the approximation delta. */
        void addApproximationClauses(EncodedApproximationDelta& delta);

        /** Calls the underlying SAT solver using the given assumption literals. */
        lbool underlyingSolve(const std::vector<Lit>& assumptions);

        /**
         * Creates a function returning the set of literals contained in clauses
         * activated by the assumption literals whose activeness provoked
         * unsatisfiability at the previous SAT solver invocation's final conflict.
         */
        std::function<std::unique_ptr<std::unordered_set<Var>>()> createConflictGetter();

        std::unique_ptr<Conjectures> m_conjectures;
        CandySolverInterface* m_solver;
        int m_maxRefinementSteps;
        std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> m_heuristics;
        SimplificationHandlingMode m_simpHandlingMode;

        std::unique_ptr<RefinementStrategy> m_refinementStrategy;

        /**
         * Problem literals by assumption literal. For backbone encodings, the
         * literals of the rsp. pair are equal.
         */
        std::unordered_map<Lit, std::pair<Lit, Lit>> m_approxLitsByAssumption;
    };

    /**
     * \class ARSolverBuilder
     *
     * \ingroup RS_AbstractionRefinement
     *
     * \brief A builder for ARSolver instances.
     */
    class ARSolverBuilder {
    public:
        /**
         * Sets the conjectures (about equivalencies between literals in the SAT problem
         * instance and the problem instance's backbone) which should be used to compute
         * approximations. The default is not to use any conjectures, reducing the
         * solving process to plain SAT solving.
         */
        virtual ARSolverBuilder& withConjectures(std::unique_ptr<Conjectures> conjectures) noexcept = 0;

        /**
         * Sets the incremental SAT solver to be used. By default, a default-constructed
         * instance of Glucose::SimpSolver is used.
         * The solver must be in a state such that
         *  - no clauses have been added yet
         *  - it is possible to call solve(...) functions
         */
        virtual ARSolverBuilder& withSolver(CandySolverInterface* solver) noexcept = 0;

        /**
         * Sets the maximum amount of refinement steps to be performed. A negative maximum
         * causes removes the bound on the maximum amount of refinement steps. By default,
         * no such bound is used.
         */
        virtual ARSolverBuilder& withMaxRefinementSteps(int maxRefinementSteps) noexcept = 0;

        /**
         * Adds the given heuristic to the set of refinement heuristics to be used to compute
         * approximations of the SAT problem instance.
         */
        virtual ARSolverBuilder& addRefinementHeuristic(std::unique_ptr<RefinementHeuristic> heuristic) = 0;

        /**
         * Sets the simplification handling mode to be used (see the documentation of
         * SimplificationHandlingMode). The default is SimplificationHandlingMode::RESTRICT.
         *
         * Note: The FREEZE simplification handling mode is currently unavailable due to an ongoing
         * refactoring of the simplification code in SimpSolver.
         */
        virtual ARSolverBuilder& withSimplificationHandlingMode(SimplificationHandlingMode mode) noexcept = 0;

        /**
         * Builds the ARSolver instance. Can be called only once.
         */
        virtual ARSolver* build() = 0;

        ARSolverBuilder() noexcept;
        virtual ~ARSolverBuilder();
        ARSolverBuilder(const ARSolverBuilder& other) = delete;
        ARSolverBuilder& operator= (const ARSolverBuilder& other) = delete;
    };

    /**
     * \ingroup RS_AbstractionRefinement
     *
     * creates an ARSolverBuilder instance.
     */
    std::unique_ptr<ARSolverBuilder> createARSolverBuilder();
}

#endif
