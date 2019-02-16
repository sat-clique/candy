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

#include "ARSolver.h"

#include "candy/core/CandySolverInterface.h"
#include "candy/frontend/CLIOptions.h"

#include <candy/randomsimulation/Conjectures.h>
#include <candy/utils/MemUtils.h>

#include "Heuristics.h"
#include "Refinement.h"

#include <unordered_map>
#include <unordered_set>
#include <iostream>

namespace Candy {
    ARSolverBuilder::ARSolverBuilder() noexcept {
        VariableEliminationOptions::opt_use_elim = false; 
    }
    
    ARSolverBuilder::~ARSolverBuilder() {
        
    }
    
    /* -- Solver-internal heuristics --------------------------------------------------------- */
    
    class ARSolverConflictHeuristic : public RefinementHeuristic {
    public:
        
        void beginRefinementStep() override;
        void markRemovals(EquivalenceImplications& equivalence) override;
        void markRemovals(Backbones& backbones) override;
        
        explicit ARSolverConflictHeuristic(std::function<std::unique_ptr<std::unordered_set<Var>>()> getConflitLits) noexcept;
        virtual ~ARSolverConflictHeuristic();
        ARSolverConflictHeuristic(const ARSolverConflictHeuristic &other) = delete;
        ARSolverConflictHeuristic& operator= (const ARSolverConflictHeuristic &other) = delete;
        
    private:
        std::function<std::unique_ptr<std::unordered_set<Var>>()> m_getConflitVars;
        std::unique_ptr<std::unordered_set<Var>> m_currentConflictVars;
    };
    
    ARSolverConflictHeuristic::ARSolverConflictHeuristic(std::function<std::unique_ptr<std::unordered_set<Var>>()> getConflitVars) noexcept
    : RefinementHeuristic(),
    m_getConflitVars(getConflitVars),
    m_currentConflictVars() {
    }
    
    ARSolverConflictHeuristic::~ARSolverConflictHeuristic() {
        
    }
    
    void ARSolverConflictHeuristic::beginRefinementStep() {
        m_currentConflictVars = m_getConflitVars();
    }
    
    void ARSolverConflictHeuristic::markRemovals(EquivalenceImplications& equivalence) {
        assert(m_currentConflictVars.get() != nullptr);
        
        if (m_currentConflictVars->size() == 0) {
            return;
        }
        
        for (Implication impl : equivalence) {
            Var first = var(impl.first);
            if (m_currentConflictVars->find(first) != m_currentConflictVars->end()) {
                equivalence.addVariableRemovalToWorkQueue(first);
                m_currentConflictVars->erase(first);
            }
        }
    }
    
    void ARSolverConflictHeuristic::markRemovals(Backbones& backbones) {
        assert(m_currentConflictVars.get() != nullptr);
        
        if (m_currentConflictVars->size() == 0) {
            return;
        }
        
        for (Lit bbLit : backbones) {
            Var first = var(bbLit);
            if (m_currentConflictVars->find(first) != m_currentConflictVars->end()) {
                backbones.addVariableRemovalToWorkQueue(first);
                m_currentConflictVars->erase(first);
            }
        }
    }
    
    class ARSolverGarbageCollectorHeuristic : public RefinementHeuristic {
    public:
        
        void beginRefinementStep() override;
        void markRemovals(EquivalenceImplications& equivalence) override;
        void markRemovals(Backbones& backbones) override;
        
        ARSolverGarbageCollectorHeuristic(CandySolverInterface* solver, bool stopAfterSecondRound) noexcept;
        virtual ~ARSolverGarbageCollectorHeuristic();
        ARSolverGarbageCollectorHeuristic(const ARSolverGarbageCollectorHeuristic &other) = delete;
        ARSolverGarbageCollectorHeuristic& operator= (const ARSolverGarbageCollectorHeuristic &other) = delete;
        
    private:
        CandySolverInterface* m_solver;
        bool m_stopAfterSecondRound;
        int m_round;
    };
    
    ARSolverGarbageCollectorHeuristic::ARSolverGarbageCollectorHeuristic(CandySolverInterface* solver,
                                                                        bool stopAfterSecondRound) noexcept
    : RefinementHeuristic(),
    m_solver(solver),
    m_stopAfterSecondRound(stopAfterSecondRound),
    m_round(0) {
    }
    
    ARSolverGarbageCollectorHeuristic::~ARSolverGarbageCollectorHeuristic() {
        
    }
    
    void ARSolverGarbageCollectorHeuristic::beginRefinementStep() {
        ++m_round;
    }
    
    void ARSolverGarbageCollectorHeuristic::markRemovals(EquivalenceImplications& equivalence) {
        if (!m_stopAfterSecondRound || (m_round < 3)) {
            for (Implication impl : equivalence) {
                Var first = var(impl.first);
                equivalence.addVariableRemovalToWorkQueue(first);
            }
        }
    }
    
    void ARSolverGarbageCollectorHeuristic::markRemovals(Backbones& backbones) {
        if (!m_stopAfterSecondRound || (m_round < 3)) {
            for (Lit bbLit : backbones) {
                Var first = var(bbLit);
                backbones.addVariableRemovalToWorkQueue(first);
            }
        }
    }
    
    ARSolver::ARSolver() {

    }
    
    ARSolver::ARSolver(std::unique_ptr<Conjectures> conjectures,
    							CandySolverInterface* solver,
                                int maxRefinementSteps,
                                std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> heuristics,
                                SimplificationHandlingMode simpHandlingMode)
    : maxVars(0), 
    m_conjectures(std::move(conjectures)),
    m_solver(solver),
    m_maxRefinementSteps(maxRefinementSteps),
    m_heuristics(std::move(heuristics)),
    m_simpHandlingMode(simpHandlingMode),
    m_refinementStrategy(),
    m_approxLitsByAssumption() {
    }
    
    ARSolver::~ARSolver() {
    }
    
    lbool ARSolver::underlyingSolve(const std::vector<Lit>& assumptions) {
        m_solver->setAssumptions(assumptions);
        return m_solver->solve();
    }
    
    void ARSolver::addApproximationClauses(EncodedApproximationDelta& delta) {
        CNFProblem approximation;
        for (auto&& clause : delta.getNewClauses()) {
            auto assumptionLit = getAssumptionLit(clause);
            auto otherLits = getNonAssumptionLits(clause);
            m_approxLitsByAssumption[assumptionLit] = otherLits;
            approximation.readClause((Cl&)clause);
        }
        m_solver->init(approximation);
    }

    /** Marks all clauses deactivated in the given approximation delta. */
    static const std::vector<Lit> deactivatedAssumptions(EncodedApproximationDelta& delta) {
        std::vector<Lit> result;
        for (auto lit : delta.getAssumptionLiterals()) {
            result.push_back(deactivatedAssumptionLit(var(lit)));
        }
        return result;
    }

    void ARSolver::reduceConjectures() {
        auto newConjectures = std::unique_ptr<Conjectures>(new Conjectures());
        for (auto&& equivalenceConj : m_conjectures->getEquivalences()) {
            EquivalenceConjecture copy;
            for (auto lit : equivalenceConj) {
                copy.addLit(lit);
            }
            if (copy.size() > 1) {
                newConjectures->addEquivalence(copy);
            }
        }
        
        for (auto backboneConj : m_conjectures->getBackbones()) {
            newConjectures->addBackbone(backboneConj);
        }
        
        m_conjectures = std::move(newConjectures);
    }
    
    std::function<std::unique_ptr<std::unordered_set<Var>>()> ARSolver::createConflictGetter() {
        return [this]() {
            auto result = std::unique_ptr<std::unordered_set<Var>>(new std::unordered_set<Var>());
            
            for (auto lit : m_solver->getConflict()) {
                assert(this->m_approxLitsByAssumption.find(lit) != this->m_approxLitsByAssumption.end());
                auto lits = this->m_approxLitsByAssumption[lit];
                result->insert(var(lits.first));
                result->insert(var(lits.second));
            }
            
            return result;
        };
    }
    
    void ARSolver::initialize() {
        // In FULL simp. handling mode, simplify here to avoid adding unneccessary variables to the
        // approximation computation system.
        if (m_simpHandlingMode == SimplificationHandlingMode::FULL) {
            // Currently not supported:
            // eliminate();
            // reduceConjectures();
        }
        
        // The latest point of simplification is right after inserting the clauses of the
        // first approximation. Eliminated variables may linger on in the approximation
        // computation system. Therefore, eliminated variables are removed during the first
        // two approximation computation stages.
        auto gcHeuristic = std::unique_ptr<RefinementHeuristic>(new ARSolverGarbageCollectorHeuristic(m_solver, true));
        m_heuristics->push_back(std::move(gcHeuristic));
        
        // Always remove equivalencies and backbones of which it is known that they are
        // causing unsatisfiability.
        auto conflictGetter = createConflictGetter();
        auto conflictHeuristic = std::unique_ptr<RefinementHeuristic>(new ARSolverConflictHeuristic(conflictGetter));
        m_heuristics->push_back(std::move(conflictHeuristic));
        
        
        // set up the refinement strategy
        m_refinementStrategy = createDefaultRefinementStrategy(*m_conjectures,
                                                               std::move(m_heuristics),
                                                               [this]() {
                                                                   return this->createVariable();
                                                               });
    }
    
    lbool ARSolver::solve() {
        if (m_maxRefinementSteps == 0) {
            // no refinement allowed -> use plain sat solving
            return m_solver->solve();
        }
        
        initialize();
        
        lbool sat = l_False;
        bool abort = false;
        int i = 0;
        std::unique_ptr<EncodedApproximationDelta> currentDelta;
        do {
            if (i == 0) {
                currentDelta = m_refinementStrategy->init();
                addApproximationClauses(*currentDelta);

                if (m_simpHandlingMode == SimplificationHandlingMode::FREEZE) {
                    // freeze the variables occuring in the delta, so that they won't be eliminated due to simplification.
                    for (auto&& clause : currentDelta->getNewClauses()) {
                        auto lits = getNonAssumptionLits(clause);
                        m_solver->setFrozen(var(lits.first), true);
                        m_solver->setFrozen(var(lits.second), true);
                    }
                }
            }
            else if (i != m_maxRefinementSteps) {
                // Note that m_maxRefinementSteps != 0
                currentDelta = m_refinementStrategy->refine();
                addApproximationClauses(*currentDelta);
            }
            
            if (m_maxRefinementSteps < 0 || i < m_maxRefinementSteps) {
                // can perform another step after this one => use the approximation clauses.
                assert(currentDelta.get() != nullptr);
                
                sat = underlyingSolve(currentDelta->getAssumptionLiterals());
                                
                // the loop can safely be exited if no approximation clauses were used during solve
                abort |= (sat != l_True) || (currentDelta->countEnabledClauses() == 0);
            }
            else {
                // no further steps after this one => deactivate the approximation clauses completely.
                assert(currentDelta.get() != nullptr);
                auto assumptions = deactivatedAssumptions(*currentDelta);
                sat = underlyingSolve(assumptions);
                abort = true;
            }
            
            ++i;
        } while(!abort && (sat == l_False));
        return sat;
    }
    
    /* -- ARSolverBuilder implementation --------------------------------------------------------- */
    
    class ARSolverBuilderImpl : public ARSolverBuilder {
    public:
        ARSolverBuilderImpl& withConjectures(std::unique_ptr<Conjectures> conjectures) noexcept override;
        ARSolverBuilderImpl& withSolver(CandySolverInterface* solver) noexcept override;
        ARSolverBuilderImpl& withMaxRefinementSteps(int maxRefinementSteps) noexcept override;
        ARSolverBuilderImpl& addRefinementHeuristic(std::unique_ptr<RefinementHeuristic> heuristic) override;
        ARSolverBuilderImpl& withSimplificationHandlingMode(SimplificationHandlingMode mode) noexcept override;
        
        ARSolver* build() override;
        
        ARSolverBuilderImpl() noexcept;
        virtual ~ARSolverBuilderImpl();
        ARSolverBuilderImpl(const ARSolverBuilderImpl& other) = delete;
        ARSolverBuilderImpl& operator=(const ARSolverBuilderImpl& other) = delete;
        
    private:
        std::unique_ptr<Conjectures> m_conjectures;
        CandySolverInterface* m_solver;
        int m_maxRefinementSteps;
        std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> m_heuristics;
        SimplificationHandlingMode m_simpHandlingMode;
        
        bool m_called;
    };
    
    ARSolverBuilderImpl::ARSolverBuilderImpl() noexcept : ARSolverBuilder(),
    m_conjectures(),
    m_solver(),
    m_maxRefinementSteps(-1),
    m_heuristics(),
    m_simpHandlingMode(SimplificationHandlingMode::FREEZE),
    m_called(false) {
        m_heuristics.reset(new std::vector<std::unique_ptr<RefinementHeuristic>>);
        
    }
    
    ARSolverBuilderImpl::~ARSolverBuilderImpl() {
        
    }
    
    ARSolverBuilderImpl& ARSolverBuilderImpl::withConjectures(std::unique_ptr<Conjectures> conjectures) noexcept {
        m_conjectures = std::move(conjectures);
        return *this;
    }
    
    ARSolverBuilderImpl& ARSolverBuilderImpl::withSolver(CandySolverInterface* solver) noexcept {
        m_solver = solver;
        return *this;
    }
    
    ARSolverBuilderImpl& ARSolverBuilderImpl::withMaxRefinementSteps(int maxRefinementSteps) noexcept {
        m_maxRefinementSteps = maxRefinementSteps;
        return *this;
    }
    
    ARSolverBuilderImpl& ARSolverBuilderImpl::addRefinementHeuristic(std::unique_ptr<RefinementHeuristic> heuristic) {
        m_heuristics->push_back(std::move(heuristic));
        return *this;
    }
    
    ARSolverBuilderImpl& ARSolverBuilderImpl::withSimplificationHandlingMode(SimplificationHandlingMode mode) noexcept {
        m_simpHandlingMode = mode;
        return *this;
    }
    
    ARSolver* ARSolverBuilderImpl::build() {
        assert(!m_called);
        m_called = true;
        
        std::unique_ptr<Conjectures> usedConjectures = std::move(m_conjectures);
        if (usedConjectures.get() == nullptr) {
            usedConjectures.reset(new Conjectures());
        }

        ARSolver* result = new ARSolver(std::move(usedConjectures),
                                                           m_solver,
                                                           m_maxRefinementSteps,
                                                           std::move(m_heuristics),
                                                           m_simpHandlingMode);
        return result;
    }


    std::unique_ptr<ARSolverBuilder> createARSolverBuilder() {
        return backported_std::make_unique<ARSolverBuilderImpl>();
    }
}
