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

#ifndef X_66055FCA_E0CE_46B3_9E4C_7F093C37330B_BRANCHINGHEURISTICS_H
#define X_66055FCA_E0CE_46B3_9E4C_7F093C37330B_BRANCHINGHEURISTICS_H

#include <candy/core/SolverTypes.h>
#include <candy/rsil/ImplicitLearningAdvice.h>
#include <candy/randomsimulation/Conjectures.h>
#include <candy/utils/FastRand.h>

#include <type_traits>

namespace Candy {
    /**
     * \defgroup RS_ImplicitLearning
     */
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A PickBranchLitT type (see candy/core/Solver.h) for plain random-simulation-based
     * implicit learning solvers. A pickBranchLit implementation is provided for the
     * RSILBranchingHeuristic PickBranchLitT type.
     */
    template<class AdviceType>
    class RSILBranchingHeuristic {
        static_assert(std::is_class<typename AdviceType::BasicType>(), "AdviceType must have an inner type BasicType");
        
    public:
        using TrailType = std::vector<Lit>;
        using TrailLimType = std::vector<uint32_t>;
        using DecisionType = std::vector<char>;
        using AssignsType = std::vector<lbool>;
        
        using BasicType = RSILBranchingHeuristic<AdviceEntry<3>>;
        
        /**
         * RSILBranchingHeuristic parameters.
         */
        class Parameters {
        public:
            /// The conjectures to be used for implicit learning, e.g. obtained via random simulation.
            const Conjectures& conjectures;
        };
        
        RSILBranchingHeuristic();
        explicit RSILBranchingHeuristic(const Parameters& params);
        RSILBranchingHeuristic(RSILBranchingHeuristic&& other) = default;
        RSILBranchingHeuristic& operator=(RSILBranchingHeuristic&& other) = default;
        
        Lit getAdvice(const TrailType& trail,
                      const TrailLimType& trailLimits,
                      const AssignsType& assigns,
                      const DecisionType& decision) noexcept;
        
        FastRandomNumberGenerator& getRandomNumberGenerator() noexcept;
        
    protected:
        ImplicitLearningAdvice<AdviceType> m_advice;
        
    private:
        FastRandomNumberGenerator m_rng;
    };
    
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A specialization of RSILBranchingHeuristic<n> with n=3.
     */
    using RSILBranchingHeuristic3 = RSILBranchingHeuristic<AdviceEntry<3>>;
    
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * This is a RSILBranchingHeuristic tailored for implicit learning with implication
     * budgets. Each implication is assigned an initial budget, and each time it is used
     * for implicit learning, the budget is decreased by one. If an implication's budget
     * is 0, it is not further used for implicit learning.
     */
    template<unsigned int tAdviceSize>
    class RSILBudgetBranchingHeuristic : public RSILBranchingHeuristic<BudgetAdviceEntry<tAdviceSize>> {
        static_assert(tAdviceSize > 1, "advice size must be >= 2");
    public:
        
        using BasicType = RSILBudgetBranchingHeuristic<3>;
        
        /**
         * RSILBudgetBranchingHeuristic parameters.
         */
        class Parameters {
        public:
            /// The parameters for the underlying RSILBranchingHeuristic.
            typename RSILBranchingHeuristic<BudgetAdviceEntry<tAdviceSize>>::Parameters rsilParameters;
            
            /// The initial implication budget.
            uint64_t initialBudget;
            
            Parameters() = default;
            Parameters(const Conjectures& conjectures, uint64_t initialBudget = 10000ul)
            : rsilParameters({conjectures}), initialBudget(initialBudget) {
            }
        };
        
        RSILBudgetBranchingHeuristic();
        explicit RSILBudgetBranchingHeuristic(const Parameters& params);
        RSILBudgetBranchingHeuristic(RSILBudgetBranchingHeuristic&& other) = default;
        RSILBudgetBranchingHeuristic& operator=(RSILBudgetBranchingHeuristic&& other) = default;
    };
    
    using RSILBudgetBranchingHeuristic3 = RSILBudgetBranchingHeuristic<3>;
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A PickBranchLitT type (see candy/core/Solver.h) for plain random-simulation-based
     * implicit learning solvers. This heuristic produces results with decreased probability,
     * configured via the parameter \p probHalfLife : every \p probHalfLife decisions, the
     * probability of possibly overriding the solver's internal decision heuristic is halved.
     *
     * A pickBranchLit implementation is provided for the RSILVanishingBranchingHeuristic
     * PickBranchLitT type.
     */
    template<class AdviceType>
    class RSILVanishingBranchingHeuristic {
    public:
        using TrailType = typename RSILBranchingHeuristic<AdviceType>::TrailType;
        using TrailLimType = typename RSILBranchingHeuristic<AdviceType>::TrailLimType;
        using DecisionType = typename RSILBranchingHeuristic<AdviceType>::DecisionType;
        using AssignsType = typename RSILBranchingHeuristic<AdviceType>::AssignsType;
        
        
        using BasicType = RSILVanishingBranchingHeuristic<AdviceEntry<3>>;
        
        /**
         * RSILVanishingBranchingHeuristic parameters.
         */
        class Parameters {
        public:
            /// The parameters for the underlying RSILBranchingHeuristic.
            typename RSILBranchingHeuristic<AdviceType>::Parameters rsilParameters;
            
            /// The intervention probability half-life.
            uint64_t probHalfLife;
            
            Parameters() = default;
            
            Parameters(const Conjectures& conjectures) : rsilParameters({conjectures}), probHalfLife(100ull) {
            }
            
            Parameters(const Conjectures& conjectures, uint64_t probHalfLife)
            : rsilParameters({conjectures}),
            probHalfLife(probHalfLife) {}
            
            Parameters(const typename RSILBranchingHeuristic<AdviceType>::Parameters& parameters,
                       uint64_t probHalfLife) : rsilParameters(parameters), probHalfLife(probHalfLife) {
            }
        };
        
        RSILVanishingBranchingHeuristic();
        explicit RSILVanishingBranchingHeuristic(const Parameters& params);
        RSILVanishingBranchingHeuristic(RSILVanishingBranchingHeuristic&& other) = default;
        RSILVanishingBranchingHeuristic& operator=(RSILVanishingBranchingHeuristic&& other) = default;
        
        Lit getAdvice(const TrailType& trail,
                      const TrailLimType& trailLimits,
                      const AssignsType& assigns,
                      const DecisionType& decision) noexcept;
        
        
        
    private:
        bool isRSILEnabled() noexcept;
        void updateCallCounter() noexcept;
        
        uint64_t m_callCounter;
        fastnextrand_state_t m_mask;
        uint64_t m_probHalfLife;
        RSILBranchingHeuristic<AdviceType> m_rsilHeuristic;
    };
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A specialization of RSILVanishingBranchingHeuristic<n> with n=3.
     */
    using RSILVanishingBranchingHeuristic3 = RSILVanishingBranchingHeuristic<AdviceEntry<3>>;
    
    
    
    
    
    //******* Implementation ************************************************************
     
    namespace BranchingHeuristicsImpl {
        static inline Var getMaxVar(const Conjectures& conj) {
            Var result = 0;
            for (auto& c : conj.getBackbones()) {
                result = std::max(result, var(c.getLit()));
            }
            for (auto& c : conj.getEquivalences()) {
                for (auto literal : c) {
                    result = std::max(result, var(literal));
                }
            }
            return result;
        }
        
        // The following provides an implementation of usedAdvice() and canUseAdvice() for each supported advice entry
        // type, via SFINAE. It is required that ...AdviceEntry<n>::BasicType == ...AdviceEntry<m>::BasicType for all
        // admissible values of n and m. Therefore, it suffices to compare types with ...AdviceEntry<2>::BasicType.
        
        template<typename AdviceType>
        static inline
        typename std::enable_if<std::is_same<typename AdviceType::BasicType, AdviceEntry<2>::BasicType>::value, void>::type
        usedAdvice(AdviceType& advice, size_t index) noexcept {
            (void)advice;
            (void)index;
        }
        
        template<typename AdviceType>
        static inline
        typename std::enable_if<std::is_same<typename AdviceType::BasicType, AdviceEntry<2>::BasicType>::value, bool>::type
        canUseAdvice(AdviceType& advice, size_t index) noexcept {
            (void)advice;
            (void)index;
            return true;
        }
        
        template<typename AdviceType>
        static inline
        typename std::enable_if<std::is_same<typename AdviceType::BasicType, BudgetAdviceEntry<2>::BasicType>::value, void>::type
        usedAdvice(AdviceType& advice, size_t index) noexcept {
            assert (index < advice.getSize());
            assert (advice.getBudget(index) > 0);
            advice.setBudget(index, advice.getBudget(index)-1);
        }
        
        template<typename AdviceType>
        static inline
        typename std::enable_if<std::is_same<typename AdviceType::BasicType, BudgetAdviceEntry<2>::BasicType>::value, bool>::type
        canUseAdvice(AdviceType& advice, size_t index) noexcept {
            assert (index < advice.getSize());
            return advice.getBudget(index) > 0;
        }
    }
    
    
    
    template<class AdviceType>
    RSILBranchingHeuristic<AdviceType>::RSILBranchingHeuristic() : m_advice(), m_rng(0xFFFF) {
    }
    
    template<class AdviceType>
    RSILBranchingHeuristic<AdviceType>::RSILBranchingHeuristic(const RSILBranchingHeuristic::Parameters& params)
    : m_advice(params.conjectures,
               BranchingHeuristicsImpl::getMaxVar(params.conjectures)),
    m_rng(0xFFFF) {
    }
    
    template<class AdviceType>
    inline FastRandomNumberGenerator& RSILBranchingHeuristic<AdviceType>::getRandomNumberGenerator() noexcept {
        return m_rng;
    }
    
    template<class AdviceType>
    __attribute__((always_inline))
    inline Lit RSILBranchingHeuristic<AdviceType>::getAdvice(const TrailType& trail,
                                                              const TrailLimType& trailLimits,
                                                              const AssignsType& assigns,
                                                              const DecisionType& decision) noexcept {
        if (trailLimits.size() == 0) {
            return lit_Undef;
        }
        
        auto randomNumber = m_rng();
        auto trailStart = trailLimits.back();
        auto trailSize = trail.size();
        
        if (trailSize - trailStart > 16) {
            trailStart = trailSize - 16;
        }
        
        auto scanLen = trailSize - trailStart;
        auto rnd = randomNumber & 0xFFFFFFFF;
        
        for (decltype(scanLen) j = 0; j < scanLen; ++j) {
            auto i = trailStart + ((j + rnd) % scanLen);
            
            Lit cursor = trail[i];
            Var variable = var(cursor);
            
            if (!m_advice.hasPotentialAdvice(variable)) {
                continue;
            }
            
            auto& advice = m_advice.getAdvice(variable);
            
            for (decltype(advice.getSize()) a = 0; a < advice.getSize(); ++a) {
                size_t idx = (a + rnd) % advice.getSize();
                
                auto advisedLit = advice.getLiteral(idx);
                if (assigns[var(advisedLit)] == l_Undef
                    && decision[var(advisedLit)]
                    && BranchingHeuristicsImpl::canUseAdvice(advice, idx)) {
                    BranchingHeuristicsImpl::usedAdvice(advice, idx);
                    auto result = sign(cursor) ? ~advisedLit : advisedLit;
                    return result;
                }
            }
        }
        
        return lit_Undef;
    }
    
    
    
    template<unsigned int tAdviceSize>
    RSILBudgetBranchingHeuristic<tAdviceSize>::RSILBudgetBranchingHeuristic()
    : RSILBranchingHeuristic<BudgetAdviceEntry<tAdviceSize>>() {
    }
    
    template<unsigned int tAdviceSize>
    RSILBudgetBranchingHeuristic<tAdviceSize>::RSILBudgetBranchingHeuristic(const Parameters& params)
    : RSILBranchingHeuristic<BudgetAdviceEntry<tAdviceSize>>(params.rsilParameters) {
        for (Var i = 0; this->m_advice.hasPotentialAdvice(i); ++i) {
            auto& advice = this->m_advice.getAdvice(i);
            for (size_t j = 0; j < advice.getSize(); ++j) {
                advice.setBudget(j, params.initialBudget);
            }
        }
    }
    
    // Note: The RSILBudgetBranchingHeuristic implementation itself is not concerned with
    // actually updating the AdviceEntry objects, and does not directly alter the behaviour
    // of getAdvice(). Any changes to the behaviour of getAdvice() (selecting implications,
    // updating advice entries) are realized by specializations of usedAdvice() and
    // canUseAdvice() for BudgetAdviceEntry - see the BranchingHeuristicsImpl namespace
    // for further details.
    
    
    
    
    
    template<class AdviceType>
    RSILVanishingBranchingHeuristic<AdviceType>::RSILVanishingBranchingHeuristic()
    : m_callCounter(1ull),
    m_mask(0ull),
    m_probHalfLife(1ull),
    m_rsilHeuristic(){
    }
    
    template<class AdviceType>
    RSILVanishingBranchingHeuristic<AdviceType>::RSILVanishingBranchingHeuristic(const RSILVanishingBranchingHeuristic::Parameters& params)
    : m_callCounter(params.probHalfLife),
    m_mask(0ull),
    m_probHalfLife(params.probHalfLife),
    m_rsilHeuristic(params.rsilParameters) {
        assert(m_probHalfLife > 0);
    }
    
    template<class AdviceType>
    inline bool RSILVanishingBranchingHeuristic<AdviceType>::isRSILEnabled() noexcept {
        auto randomNumber = m_rsilHeuristic.getRandomNumberGenerator()();
        return (randomNumber & m_mask) == 0ull;
    }
    
    template<class AdviceType>
    inline void RSILVanishingBranchingHeuristic<AdviceType>::updateCallCounter() noexcept {
        assert(m_callCounter > 0);
        --m_callCounter;
        if (m_callCounter == 0) {
            m_mask = (m_mask << 1) | 1ull;
            m_callCounter = m_probHalfLife;
        }
    }
    
    template<class AdviceType>
    __attribute__((always_inline))
    inline Lit RSILVanishingBranchingHeuristic<AdviceType>::getAdvice(const TrailType& trail,
                                                                       const TrailLimType& trailLimits,
                                                                       const AssignsType& assigns,
                                                                       const DecisionType& decision) noexcept {
        auto result = isRSILEnabled() ? m_rsilHeuristic.getAdvice(trail, trailLimits, assigns, decision) : lit_Undef;
        updateCallCounter();
        return result;
    }
}

#endif
