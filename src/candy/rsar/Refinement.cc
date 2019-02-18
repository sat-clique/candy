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

#include "Refinement.h"
#include "Heuristics.h"
#include "Refinement.h"
#include "ApproximationState.h"


#include <candy/randomsimulation/Conjectures.h>
#include <candy/utils/MemUtils.h>

#include <memory>
#include <functional>
#include <unordered_map>

namespace Candy {

    EncodedApproximationDelta::EncodedApproximationDelta() noexcept {
    }
    
    EncodedApproximationDelta::~EncodedApproximationDelta() {
    }
    
    RefinementStrategy::RefinementStrategy() noexcept {
    }
    
    RefinementStrategy::~RefinementStrategy() {
    }
    
    class EncodedApproximationDeltaImpl : public EncodedApproximationDelta {
    public:
        const std::vector<Cl>& getNewClauses() const noexcept override;
        const std::vector<Lit>& getAssumptionLiterals() const noexcept override;
        size_t countEnabledClauses() const noexcept override;
        
        explicit EncodedApproximationDeltaImpl(std::unique_ptr<std::vector<Cl>> newClauses,
                                   const std::vector<Lit>& assumptionLiterals);
        
        virtual ~EncodedApproximationDeltaImpl();
        EncodedApproximationDeltaImpl(const EncodedApproximationDeltaImpl& other) = delete;
        EncodedApproximationDeltaImpl& operator= (const EncodedApproximationDeltaImpl& other) = delete;
        
    private:
        std::unique_ptr<std::vector<Cl>> m_newClauses;
        const std::vector<Lit>&  m_assumptionLiterals;
        size_t m_activeClauseCount;
    };
    
    EncodedApproximationDeltaImpl::~EncodedApproximationDeltaImpl() {
        
    }
    
    EncodedApproximationDeltaImpl::EncodedApproximationDeltaImpl(std::unique_ptr<std::vector<Cl>> newClauses,
                                                                 const std::vector<Lit>& assumptionLiterals) :
    EncodedApproximationDelta(),
    m_newClauses(std::move(newClauses)),
    m_assumptionLiterals(assumptionLiterals),
    m_activeClauseCount(0) {
        m_activeClauseCount = std::count_if(assumptionLiterals.begin(),
                                            assumptionLiterals.end(),
                                            [](Lit l) { return isActive(l); });
    }
    
    
    const std::vector<Cl>& EncodedApproximationDeltaImpl::getNewClauses() const noexcept {
        return *m_newClauses;
    }
    
    const std::vector<Lit>& EncodedApproximationDeltaImpl::getAssumptionLiterals() const noexcept {
        return m_assumptionLiterals;
    }
    
    size_t EncodedApproximationDeltaImpl::countEnabledClauses() const noexcept {
        return m_activeClauseCount;
    }

    
    
    
    class SimpleRefinementStrategy : public RefinementStrategy {
    public:
        std::unique_ptr<EncodedApproximationDelta> init() override;
        std::unique_ptr<EncodedApproximationDelta> refine() override;
        
        explicit SimpleRefinementStrategy(const Conjectures& conjectures,
                                 std::vector<std::unique_ptr<RefinementHeuristic>> heuristics,
                                 std::function<Var()> createVariable);
        virtual ~SimpleRefinementStrategy();
        SimpleRefinementStrategy(const SimpleRefinementStrategy& other) = delete;
        SimpleRefinementStrategy& operator= (const SimpleRefinementStrategy& other) = delete;
        
    private:
        /** Updates the approximation state using the refinement heuristics. */
        void updateApproximationState();
        
        /** Updates the assumption literal vector for the already-encoded 
         * items in the given delta (i.e. not the new implications). */
        void updateAssumptionLiterals(ApproximationDelta& delta);
        
        /** Translates the given ApproximationDelta to an EncodedApproximationDelta.
         * Sets up assumption literals for the new clauses. 
         */
        std::unique_ptr<EncodedApproximationDelta> encode(ApproximationDelta& delta);
        
        /** Translates the given ApproximationDelta to an EncodedApproximationDelta,
         * omitting clause removals. Sets up the initial assumption literals.
         */
        std::unique_ptr<EncodedApproximationDelta> encodeInitial(ApproximationDelta& delta);
        
        /** Creates a new assumption variable. */
        Var addAssumptionVariable();
        
        
        std::vector<std::unique_ptr<RefinementHeuristic>> m_heuristics;
        std::unique_ptr<ApproximationState> m_approximationState;
        std::function<Var()> m_createVariable;
        
        /** The current collection of assumption literals, marking the approximation clauses'
         * "activeness" states. */
        std::vector<Lit> m_assumptionLits;
        
        /** Maps implications to the index of their rsp. assumption literal in m_assumptionLits. */
        std::unordered_map<Implication, std::vector<Lit>::size_type> m_assumptionLitsIdxByImpl;
        
        /** Maps backbone lits to the index of their rsp. assumption literal in m_assumptionLits. */
        std::unordered_map<Lit, std::vector<Lit>::size_type> m_assumptionLitsIdxByBackbone;
    };
    
    SimpleRefinementStrategy::~SimpleRefinementStrategy() {
        
    }
    
    SimpleRefinementStrategy::SimpleRefinementStrategy(const Conjectures& conjectures,
                                                       std::vector<std::unique_ptr<RefinementHeuristic>> heuristics,
                                                       std::function<Var()> createVariable)
    : RefinementStrategy(),
    m_heuristics(std::move(heuristics)),
    m_approximationState(createApproximationState(conjectures)),
    m_createVariable(createVariable),
    m_assumptionLits(),
    m_assumptionLitsIdxByImpl(),
    m_assumptionLitsIdxByBackbone() {
    }
    
    
    Var SimpleRefinementStrategy::addAssumptionVariable() {
        Var assumptionVariable = m_createVariable();
        m_assumptionLits.push_back(activatedAssumptionLit(assumptionVariable));
        return assumptionVariable;
    }
    
    
    
    void SimpleRefinementStrategy::updateApproximationState() {
        bool markBackbones = !(m_approximationState->getBackbones().empty());
        
        for (auto& heuristic : m_heuristics) {
            heuristic->beginRefinementStep();
            if (markBackbones) {
                heuristic->markRemovals(m_approximationState->getBackbones());
            }
        }
        
        for (auto iter = m_approximationState->beginEquivalenceImplications();
             iter != m_approximationState->endEquivalenceImplications(); ++iter) {
            for (auto& heuristic : m_heuristics) {
                heuristic->markRemovals(**iter);
            }
        }
    }
    
    void SimpleRefinementStrategy::updateAssumptionLiterals(ApproximationDelta& delta) {
        for (auto bbIter = delta.beginRemovedBackbones();
             bbIter != delta.endRemovedBackbones(); ++bbIter) {
            auto assumptionLitIdx = m_assumptionLitsIdxByBackbone[*bbIter];
            auto& assumptionLit = m_assumptionLits[assumptionLitIdx];
            assumptionLit = deactivatedAssumptionLit(var(assumptionLit));
        }
        
        for (auto implIter = delta.beginRemovedImplications();
             implIter != delta.endRemovedImplications(); ++implIter) {
            auto assumptionLitIdx = m_assumptionLitsIdxByImpl[*implIter];
            auto& assumptionLit = m_assumptionLits[assumptionLitIdx];
            assumptionLit = deactivatedAssumptionLit(var(assumptionLit));
        }
    }
    
    
    
    std::unique_ptr<EncodedApproximationDelta> SimpleRefinementStrategy::encodeInitial(Candy::ApproximationDelta &delta) {
        auto encodedDelta = encode(delta);
        
        auto clauses = std::unique_ptr<std::vector<Cl>>(new std::vector<Cl>(encodedDelta->getNewClauses()));
        
        for (auto backboneConj : m_approximationState->getBackbones()) {
            Var assumptionVar = addAssumptionVariable();
            clauses->push_back(encodeBackbone(BackboneConjecture{backboneConj}, assumptionVar));
            m_assumptionLitsIdxByBackbone[backboneConj] = m_assumptionLits.size()-1;
        }
        
        return std::unique_ptr<EncodedApproximationDelta>(new EncodedApproximationDeltaImpl(std::move(clauses), m_assumptionLits));
    }
    
    std::unique_ptr<EncodedApproximationDelta> SimpleRefinementStrategy::encode(ApproximationDelta& delta) {
        auto clauses = std::unique_ptr<std::vector<Candy::Cl>>(new std::vector<Cl>());
        
        for (auto equivalence = delta.beginAddedImplications(); equivalence != delta.endAddedImplications();
             ++equivalence) {
            Candy::Var assumptionVar = addAssumptionVariable();
            clauses->push_back(encodeImplication(*equivalence, assumptionVar));
            m_assumptionLitsIdxByImpl[*equivalence] = m_assumptionLits.size()-1;
        }
        
        return std::unique_ptr<EncodedApproximationDelta>(new EncodedApproximationDeltaImpl(std::move(clauses), m_assumptionLits));
    }
    

    
    
    std::unique_ptr<EncodedApproximationDelta> SimpleRefinementStrategy::init() {
        updateApproximationState();
        auto refinementDelta = m_approximationState->createInitializationDelta();
        return encodeInitial(*refinementDelta);
    }
    
    std::unique_ptr<EncodedApproximationDelta> SimpleRefinementStrategy::refine() {
        updateApproximationState();
        auto refinementDelta = m_approximationState->createDelta();
        updateAssumptionLiterals(*refinementDelta);
        return encode(*refinementDelta);
    }
    
    
    std::unique_ptr<RefinementStrategy> createDefaultRefinementStrategy(const Conjectures& conjectures,
                                                                        std::vector<std::unique_ptr<RefinementHeuristic>> heuristics,
                                                                        std::function<Var()> createVariable) {
        return backported_std::make_unique<SimpleRefinementStrategy>(conjectures, std::move(heuristics), createVariable);
    }
    
    Lit activatedAssumptionLit(Var assumptionVar) {
        return mkLit(assumptionVar, 0);
    }
    
    Lit deactivatedAssumptionLit(Var assumptionVar) {
        return mkLit(assumptionVar, 1);
    }
    
    Cl encodeImplication(Implication implication, Var assumptionVar) {
        return Cl({~(implication.first), implication.second, deactivatedAssumptionLit(assumptionVar)});
    }
    
    Cl encodeBackbone(BackboneConjecture backbone, Var assumptionVar) {
        return Cl({backbone.getLit(), deactivatedAssumptionLit(assumptionVar)});
    }
    
    Lit getAssumptionLit(const Cl &clause) {
        assert(clause.size() == 2 || clause.size() == 3);
        return clause.back();
    }
    
    std::pair<Lit, Lit> getNonAssumptionLits(const Cl &clause) {
        std::pair<Lit, Lit> savedLits;
        savedLits.first = clause[0];
        savedLits.second = (clause.size() == 3 ? clause[1] : clause[0]);
        return savedLits;
    }
    
    bool isActive(Lit assumptionLit) {
        return sign(assumptionLit) == 0;
    }

}
