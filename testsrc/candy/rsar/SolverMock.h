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

#ifndef X_0816BDE6_14E6_49C2_84A9_47B3DA198C9D_SOLVERMOCK_H
#define X_0816BDE6_14E6_49C2_84A9_47B3DA198C9D_SOLVERMOCK_H

#include <candy/core/CNFProblem.h>
#include <candy/core/SolverTypes.h>
#include "candy/core/CandySolverInterface.h"
#include "candy/rsar/ARSolver.h"

#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace Candy {
    /**
     * \class SolverMockEvent
     *
     * \ingroup RS_AbstractionRefinement_Tests
     *
     * \brief An enum for events recorded by SolverMock.
     */
    enum class SolverMockEvent {
        ADD_PROBLEM,
        ADD_CLAUSE,
        SOLVE,
        SIMPLIFY
    };
    
    /**
     * \class SolverMock
     *
     * \ingroup RS_AbstractionRefinement_Tests
     *
     * \brief An elaborate mock of Glucose
     *
     * A mock Glucose, intended for testing ARSolver.
     */
    class SolverMock : public CandySolverInterface {
    public:
        
        virtual void setCertificate(Certificate& certificate) override {}
        virtual void setVerbosities(unsigned int verbEveryConflicts, unsigned int verbosity) override {}
        virtual void enablePreprocessing() override {}
        virtual void disablePreprocessing() override {}

        virtual Certificate* getCertificate() override { return nullptr; }
        virtual unsigned int getVerbosity() override { return 0; }

        virtual Var newVar() override;

        virtual void addClauses(const CNFProblem& problem) override;
        virtual bool addClause(const std::vector<Lit>& lits) override;
        virtual bool addClause(std::initializer_list<Lit> lits) override { return false; }

        virtual bool simplify() override; // remove satisfied clauses
        virtual bool strengthen() override { return true; } // remove false literals from clauses
        virtual bool eliminate() override { return true; }  // Perform variable elimination based simplification.
        virtual bool eliminate(bool use_asymm, bool use_elim) override { return true; }  // Perform variable elimination based simplification.
        virtual bool isEliminated(Var v) const override;
        virtual void setFrozen(Var v, bool freeze) override;

    	virtual lbool solve() override;
    	virtual lbool solve(std::initializer_list<Lit> assumps) override { return solve(); }
    	virtual lbool solve(const std::vector<Lit>& assumps) override { return solve(); }

    	virtual void setConfBudget(uint64_t x) override { }
    	virtual void setPropBudget(uint64_t x) override { }
    	virtual void setInterrupt(bool value) override { }
    	virtual void budgetOff() override { }

    	virtual void printDIMACS() override { }

    	// The value of a variable in the last model. The last call to solve must have been satisfiable.
    	virtual lbool modelValue(Var x) const override { return l_Undef; }
    	virtual lbool modelValue(Lit p) const override { return l_Undef; }
        virtual Cl getModel() override { return Cl(); }

    	// true means solver is in a conflicting state
    	virtual bool isInConflictingState() const override;
    	virtual std::vector<Lit>& getConflict() override;

    	virtual size_t nClauses() const override { return 0; }
    	virtual size_t nLearnts() const override { return 0; }
        virtual size_t nVars() const override;

    	// Incremental mode
        virtual void setIncrementalMode() override;
    	virtual bool isIncremental() override { return true; }

        /** Sets the literals returned by getConflict(). */
        void mockctrl_setConflictLits(const std::vector<Lit> &conflictLits);
        
        /** Returns true iff setIncremental() has been called. () */
        bool mockctrl_isIncrementalSet() const noexcept;
        
        /** Returns true iff parsing has been set to true. */
        bool mockctrl_isParsingSet() const noexcept;
        
        /** Returns true iff v is an assumption variable. */
        bool mockctrl_isAssumptionVar(Var v) const noexcept;
        
        /** Sets isElimininated(v) */
        void mockctrl_setEliminated(Var v, bool elim);
        
        /** Sets the result of the SAT solver at the nth invocation */
        void mockctrl_setResultInInvocation(int n, bool result);
        
        /** Sets the result of the SAT solver unless the result has been specified
         * via mockctrl_setResultInInvocation */
        void mockctrl_setDefaultSolveResult(bool result) noexcept;
        
        /** Gets a recording of the events registered by the mock
         * (See SolverMockEvent for the kind of events being recorded) */
        const std::vector<SolverMockEvent>& mockctrl_getEventLog() const noexcept;
        
        /** Gets a recording of the events registered by the mock
         * (See SolverMockEvent for the kind of events being recorded) */
        const std::vector<Lit>& mockctrl_getLastAssumptionLits() const noexcept;
        
        /** Sets a function to be called when a solve method gets invoked.
         * The function receives the amount of previous solve incovations
         * as an argument.
         */
        void mockctrl_callOnSolve(std::function<void(int)> func);
        
        /** Sets a function to be called when the simplify method gets
         * invoked. The function receives the amounf of previous solve
         * invocations as an argument.
         */
        void mockctrl_callOnSimplify(std::function<void(int)> func);
        
        /** Returns the amount of solve method invocations so far. */
        int mockctrl_getAmountOfInvocations() const noexcept;
        
        /** Returns the amount of clauses added since the last solve
         * invocation */
        int mockctrl_getAmountOfClausesAddedSinceLastSolve() const noexcept;
        
        /** Returns the clauses added via addClause(). */
        const std::vector<Cl> &mockctrl_getAddedClauses() const noexcept;
        
        /** Returns the min. and max. non-assumption variable. */
        std::pair<Var, Var> mockctrl_getNonAssumptionVars() const noexcept;
        
        /** Returns the min. and max. assumption variables. */
        std::pair<Var, Var> mockctrl_getAssumptionVars() const noexcept;
        
        
        SolverMock() noexcept;
        virtual ~SolverMock();
        SolverMock(const SolverMock &other) = delete;
        SolverMock& operator= (const SolverMock& other) = delete;
        
    private:
        int m_nInvocations;
        Var m_maxCreatedVar;
        std::vector<Lit> m_conflictLits;
        Var m_minAssumptionVar;
        std::unordered_set<Var> m_eliminatedVars;
        std::unordered_set<Var> m_frozenVars;
        std::unordered_map<int, bool> m_solveResultInInvocationN;
        bool m_defaultSolveResult;
        std::function<void(int)> m_callOnSolve;
        std::function<void(int)> m_callOnSimplify;
        int m_nClausesAddedSinceLastSolve;
        std::vector<Lit> m_lastAssumptionLits;
        
        std::vector<Cl> m_addedClauses;
        
        bool m_isIncrementalSet;
        bool m_isParsingSet;
        
        std::vector<SolverMockEvent> m_eventLog;
    };
    
}

#endif
