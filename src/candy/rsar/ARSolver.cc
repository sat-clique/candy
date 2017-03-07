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
#include <simp/SimpSolver.h>

#include <randomsimulation/Conjectures.h>
#include <utils/MemUtils.h>

#include "Heuristics.h"
#include "Refinement.h"
#include "SolverAdapter.h"

#include <unordered_map>
#include <unordered_set>

namespace Candy {
    ARSolver::ARSolver() noexcept {
        
    }
    
    ARSolver::~ARSolver() {
        
    }
    
    ARSolverBuilder::ARSolverBuilder() noexcept {
        
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
        
        ARSolverGarbageCollectorHeuristic(SolverAdapter& solver, bool stopAfterSecondRound) noexcept;
        virtual ~ARSolverGarbageCollectorHeuristic();
        ARSolverGarbageCollectorHeuristic(const ARSolverGarbageCollectorHeuristic &other) = delete;
        ARSolverGarbageCollectorHeuristic& operator= (const ARSolverGarbageCollectorHeuristic &other) = delete;
        
    private:
        SolverAdapter &m_solver;
        bool m_stopAfterSecondRound;
        int m_round;
    };
    
    ARSolverGarbageCollectorHeuristic::ARSolverGarbageCollectorHeuristic(SolverAdapter& solver,
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
                if (m_solver.isEliminated(first)) {
                    equivalence.addVariableRemovalToWorkQueue(first);
                }
            }
        }
    }
    
    void ARSolverGarbageCollectorHeuristic::markRemovals(Backbones& backbones) {
        if (!m_stopAfterSecondRound || (m_round < 3)) {
            for (Lit bbLit : backbones) {
                Var first = var(bbLit);
                if (m_solver.isEliminated(first)) {
                    backbones.addVariableRemovalToWorkQueue(first);
                }
            }
        }
    }

    
    
    
    
    
    /* -- ARSolver implementation --------------------------------------------------------- */
    
    class ARSolverImpl : public ARSolver {
    public:
        bool solve(CNFProblem &problem) override;
        bool solve() override;
        
        ARSolverImpl(std::unique_ptr<Conjectures> conjectures,
                     std::unique_ptr<SolverAdapter> solver,
                     int maxRefinementSteps,
                     std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> heuristics,
                     SimplificationHandlingMode simpHandlingMode);
        
        virtual ~ARSolverImpl();
        ARSolverImpl(const ARSolverImpl& other) = delete;
        ARSolverImpl& operator=(const ARSolverImpl& other) = delete;
        
    private:
        /**
         * Initializes the abstraction refinement system and performs
         * initialization-time simplification.
         */
        void init();
        
        /** Runs the underlying SAT solver's simplification system. */
        void simplify();
        
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
        bool underlyingSolve(const std::vector<Lit>& assumptions);
        
        /**
         * Creates a function returning the set of literals contained in clauses
         * activated by the assumption literals whose activeness provoked
         * unsatisfiability at the previous SAT solver invocation's final conflict.
         */
        std::function<std::unique_ptr<std::unordered_set<Var>>()> createConflictGetter();
        
        std::unique_ptr<Conjectures> m_conjectures;
        std::unique_ptr<SolverAdapter> m_solver;
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
    
    ARSolverImpl::ARSolverImpl(std::unique_ptr<Conjectures> conjectures,
                               std::unique_ptr<SolverAdapter> solver,
                               int maxRefinementSteps,
                               std::unique_ptr<std::vector<std::unique_ptr<RefinementHeuristic>>> heuristics,
                               SimplificationHandlingMode simpHandlingMode)
    : ARSolver(),
    m_conjectures(std::move(conjectures)),
    m_solver(std::move(solver)),
    m_maxRefinementSteps(maxRefinementSteps),
    m_heuristics(std::move(heuristics)),
    m_simpHandlingMode(simpHandlingMode),
    m_refinementStrategy(),
    m_approxLitsByAssumption() {
    }
    
    ARSolverImpl::~ARSolverImpl() {
    }
    
    bool ARSolverImpl::underlyingSolve(const std::vector<Lit>& assumptions) {
        // TODO: add options to further simplify between underlying solver invocations
        // if applicable
        return m_solver->solve(assumptions, false, true);
    }
    
    void ARSolverImpl::addApproximationClauses(EncodedApproximationDelta& delta) {
        for (auto&& clause : delta.getNewClauses()) {
            auto assumptionLit = getAssumptionLit(clause);
            auto otherLits = getNonAssumptionLits(clause);
            m_approxLitsByAssumption[assumptionLit] = otherLits;
            
            bool success = m_solver->addClause(clause);
            (void)success; // prevents release-mode compiler from warning about unused variable success
            assert(success);
        }
    }
    
    void ARSolverImpl::addInitialApproximationClauses(EncodedApproximationDelta& delta) {
        if (m_simpHandlingMode == SimplificationHandlingMode::FREEZE) {
            // freeze the variables occuring in the delta, so that they won't be eliminated
            // due to simplification.
            for (auto&& clause : delta.getNewClauses()) {
                for (auto lit : clause) {
                    m_solver->setFrozen(var(lit), true);
                }
            }
        }
        
        addApproximationClauses(delta);
        
        // In RESTRICT and FREEZE simp. handling mode, simplify just after inserting the clauses of the
        // first approximation stage.
        if (m_simpHandlingMode == SimplificationHandlingMode::RESTRICT
            || m_simpHandlingMode == SimplificationHandlingMode::FREEZE) {
            simplify();
        }
    }

    /** Marks all clauses deactivated in the given approximation delta. */
    static const std::vector<Lit> deactivatedAssumptions(EncodedApproximationDelta& delta) {
        std::vector<Lit> result;
        for (auto lit : delta.getAssumptionLiterals()) {
            result.push_back(deactivatedAssumptionLit(var(lit)));
        }
        return result;
    }
    
    void ARSolverImpl::simplify() {
        m_solver->simplify();
    }
    
    void ARSolverImpl::reduceConjectures() {
        auto newConjectures = std::unique_ptr<Conjectures>(new Conjectures());
        for (auto&& equivalenceConj : m_conjectures->getEquivalences()) {
            EquivalenceConjecture copy;
            for (auto lit : equivalenceConj) {
                if (!m_solver->isEliminated(var(lit))) {
                    copy.addLit(lit);
                }
            }
            if (copy.size() > 1) {
                newConjectures->addEquivalence(copy);
            }
        }
        
        for (auto backboneConj : m_conjectures->getBackbones()) {
            if (!m_solver->isEliminated(var(backboneConj.getLit()))) {
                newConjectures->addBackbone(backboneConj);
            }
        }
        
        m_conjectures = std::move(newConjectures);
    }
    
    std::function<std::unique_ptr<std::unordered_set<Var>>()> ARSolverImpl::createConflictGetter() {
        return [this]() {
            auto result = std::unique_ptr<std::unordered_set<Var>>(new std::unordered_set<Var>());
            
            for (auto lit : m_solver->getConflict()) {
                assert(this->m_approxLitsByAssumption.find(lit) != this->m_approxLitsByAssumption.end());
                auto lits = this->m_approxLitsByAssumption[lit];
                result->insert(lits.first);
                result->insert(lits.second);
            }
            
            return result;
        };
    }
    
    void ARSolverImpl::init() {
        // Set up the underlying SAT solver
        m_solver->setIncrementalMode();
        m_solver->initNbInitialVars(m_solver->getNVars());
        m_solver->setParsing(false);
        
        // In FULL simp. handling mode, simplify here to avoid adding unneccessary variables to the
        // approximation computation system.
        if (m_simpHandlingMode == SimplificationHandlingMode::FULL) {
            simplify();
            reduceConjectures();
        }
        
        // The latest point of simplification is right after inserting the clauses of the
        // first approximation. Eliminated variables may linger on in the approximation
        // computation system. Therefore, eliminated variables are removed during the first
        // two approximation computation stages.
        auto gcHeuristic = std::unique_ptr<RefinementHeuristic>(new ARSolverGarbageCollectorHeuristic(*m_solver, true));
        m_heuristics->push_back(std::move(gcHeuristic));
        
        // Always remove equivalencies and backbones of which it is known that they are
        // causing unsatisfiability.
        auto conflictGetter = createConflictGetter();
        auto conflictHeuristic = std::unique_ptr<RefinementHeuristic>(new ARSolverConflictHeuristic(conflictGetter));
        m_heuristics->push_back(std::move(conflictHeuristic));
        
        
        // set up the refinement strategy
        auto solverPtr = m_solver.get();
        m_refinementStrategy = createDefaultRefinementStrategy(*m_conjectures,
                                                               std::move(m_heuristics),
                                                               [solverPtr]() {
                                                                   auto newVar = solverPtr->newVar();
                                                                   solverPtr->setFrozen(newVar, true);
                                                                   return newVar;
                                                               });
    }

    
    bool ARSolverImpl::solve(Candy::CNFProblem &problem) {
        m_solver->insertClauses(problem);
        return solve();
    }
    
    bool ARSolverImpl::solve() {
        if (m_maxRefinementSteps == 0) {
            // no refinement allowed -> use plain sat solving
            return m_solver->solve();
        }
        
        init();
        
        bool sat = false, abort = false;
        int i = 0;
        std::unique_ptr<EncodedApproximationDelta> currentDelta;
        do {
            if (i == 0) {
                currentDelta = m_refinementStrategy->init();
                addInitialApproximationClauses(*currentDelta);
            }
            else if (i != m_maxRefinementSteps) {
                // Note that m_maxRefinementSteps != 0
                currentDelta = m_refinementStrategy->refine();
                addApproximationClauses(*currentDelta);
            }
            
            if (m_maxRefinementSteps < 0 || i < m_maxRefinementSteps) {
                // can perform another step after this one => use the approximation clauses.
                assert(currentDelta.get() != nullptr);
                sat |= underlyingSolve(currentDelta->getAssumptionLiterals());
                // the loop can safely be exited if no approximation clauses were used during solve
                abort |= (currentDelta->countEnabledClauses() == 0);
            }
            else {
                // no further steps after this one => deactivate the approximation clauses completely.
                assert(currentDelta.get() != nullptr);
                auto assumptions = deactivatedAssumptions(*currentDelta);
                sat |= underlyingSolve(assumptions);
                abort = true;
            }
            
            ++i;
        } while(!abort && !sat);
        return sat;
    }
    

    
    
    /* -- ARSolverBuilder implementation --------------------------------------------------------- */
    
    class ARSolverBuilderImpl : public ARSolverBuilder {
    public:
        ARSolverBuilderImpl& withConjectures(std::unique_ptr<Conjectures> conjectures) noexcept override;
        ARSolverBuilderImpl& withSolver(std::unique_ptr<SolverAdapter> solver) noexcept override;
        ARSolverBuilderImpl& withMaxRefinementSteps(int maxRefinementSteps) noexcept override;
        ARSolverBuilderImpl& addRefinementHeuristic(std::unique_ptr<RefinementHeuristic> heuristic) override;
        ARSolverBuilderImpl& withSimplificationHandlingMode(SimplificationHandlingMode mode) noexcept override;
        
        std::unique_ptr<ARSolver> build() override;
        
        ARSolverBuilderImpl() noexcept;
        virtual ~ARSolverBuilderImpl();
        ARSolverBuilderImpl(const ARSolverBuilderImpl& other) = delete;
        ARSolverBuilderImpl& operator=(const ARSolverBuilderImpl& other) = delete;
        
    private:
        std::unique_ptr<Conjectures> m_conjectures;
        std::unique_ptr<SolverAdapter> m_solver;
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
    m_simpHandlingMode(SimplificationHandlingMode::RESTRICT),
    m_called(false) {
        m_heuristics.reset(new std::vector<std::unique_ptr<RefinementHeuristic>>);
        
    }
    
    ARSolverBuilderImpl::~ARSolverBuilderImpl() {
        
    }
    
    ARSolverBuilderImpl& ARSolverBuilderImpl::withConjectures(std::unique_ptr<Conjectures> conjectures) noexcept {
        m_conjectures = std::move(conjectures);
        return *this;
    }
    
    ARSolverBuilderImpl& ARSolverBuilderImpl::withSolver(std::unique_ptr<SolverAdapter> solver) noexcept {
        m_solver = std::move(solver);
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
    
    std::unique_ptr<ARSolver> ARSolverBuilderImpl::build() {
        assert(!m_called);
        m_called = true;
        
        std::unique_ptr<Conjectures> usedConjectures = std::move(m_conjectures);
        if (usedConjectures.get() == nullptr) {
            usedConjectures.reset(new Conjectures());
        }
        
        std::unique_ptr<SolverAdapter> usedSolver = std::move(m_solver);
        if (usedSolver.get() == nullptr) {
            usedSolver = createGlucoseAdapter();
        }
        
        std::unique_ptr<ARSolver> result;
        result = backported_std::make_unique<ARSolverImpl>(std::move(usedConjectures),
                                                           std::move(usedSolver),
                                                           m_maxRefinementSteps,
                                                           std::move(m_heuristics),
                                                           m_simpHandlingMode);
        return result;
    }

    
    std::unique_ptr<ARSolverBuilder> createARSolverBuilder() {
        return backported_std::make_unique<ARSolverBuilderImpl>();
    }
}
