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
    template<unsigned int tAdviceSize>
    class RSILBranchingHeuristic {
    public:
        using TrailType = std::vector<Lit>;
        using TrailLimType = std::vector<uint32_t>;
        using DecisionType = std::vector<char>;
        using AssignsType = std::vector<lbool>;
        
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
        
    private:
        ImplicitLearningAdvice<AdviceEntry<tAdviceSize>> m_advice;
        FastRandomNumberGenerator m_rng;
    };
    
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A specialization of RSILBranchingHeuristic<n> with n=3.
     */
    using RSILBranchingHeuristic3 = RSILBranchingHeuristic<3>;
    
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
    template<unsigned int tAdviceSize>
    class RSILVanishingBranchingHeuristic {
    public:
        using TrailType = typename RSILBranchingHeuristic<tAdviceSize>::TrailType;
        using TrailLimType = typename RSILBranchingHeuristic<tAdviceSize>::TrailLimType;
        using DecisionType = typename RSILBranchingHeuristic<tAdviceSize>::DecisionType;
        using AssignsType = typename RSILBranchingHeuristic<tAdviceSize>::AssignsType;
        
        /**
         * RSILVanishingBranchingHeuristic parameters.
         */
        class Parameters {
        public:
            /// The parameters for the underlying RSILBranchingHeuristic.
            typename RSILBranchingHeuristic<tAdviceSize>::Parameters rsilParameters;
            
            /// The intervention probability half-life.
            uint64_t probHalfLife;
            
            Parameters() = default;
            Parameters(const Conjectures& conjectures) : rsilParameters({conjectures}), probHalfLife(100ull) {
            }
            Parameters(const typename RSILBranchingHeuristic<tAdviceSize>::Parameters& parameters,
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
        RSILBranchingHeuristic<tAdviceSize> m_rsilHeuristic;
    };
    
    /**
     * \ingroup RS_ImplicitLearning
     *
     * A specialization of RSILVanishingBranchingHeuristic<n> with n=3.
     */
    using RSILVanishingBranchingHeuristic3 = RSILVanishingBranchingHeuristic<3>;
    
    
    
    
    
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
    }
    
    template<unsigned int tAdviceSize>
    RSILBranchingHeuristic<tAdviceSize>::RSILBranchingHeuristic() : m_advice(), m_rng(0xFFFF) {
    }
    
    template<unsigned int tAdviceSize>
    RSILBranchingHeuristic<tAdviceSize>::RSILBranchingHeuristic(const RSILBranchingHeuristic::Parameters& params)
    : m_advice(params.conjectures,
               BranchingHeuristicsImpl::getMaxVar(params.conjectures)),
    m_rng(0xFFFF) {
    }
    
    template<unsigned int tAdviceSize>
    inline FastRandomNumberGenerator& RSILBranchingHeuristic<tAdviceSize>::getRandomNumberGenerator() noexcept {
        return m_rng;
    }
    
    template<unsigned int tAdviceSize>
    __attribute__((always_inline))
    inline Lit RSILBranchingHeuristic<tAdviceSize>::getAdvice(const TrailType& trail,
                                                              const TrailLimType& trailLimits,
                                                              const AssignsType& assigns,
                                                              const DecisionType& decision) noexcept {
        assert(trailLimits.size() > 0);
        
        auto randomNumber = m_rng();
        auto trailStart = trailLimits.back();
        auto trailSize = trail.size();
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
                if (assigns[var(advisedLit)] == l_Undef && decision[var(advisedLit)]) {
                    auto result = sign(cursor) ? ~advisedLit : advisedLit;
                    return result;
                }
            }
        }
        
        return lit_Undef;
    }
    
    
    
    
    template<unsigned int tAdviceSize>
    RSILVanishingBranchingHeuristic<tAdviceSize>::RSILVanishingBranchingHeuristic()
    : m_callCounter(1ull),
    m_mask(0ull),
    m_probHalfLife(1ull),
    m_rsilHeuristic(){
    }
    
    template<unsigned int tAdviceSize>
    RSILVanishingBranchingHeuristic<tAdviceSize>::RSILVanishingBranchingHeuristic(const RSILVanishingBranchingHeuristic::Parameters& params)
    : m_callCounter(params.probHalfLife),
    m_mask(0ull),
    m_probHalfLife(params.probHalfLife),
    m_rsilHeuristic(params.rsilParameters) {
        assert(m_probHalfLife > 0);
    }
    
    template<unsigned int tAdviceSize>
    inline bool RSILVanishingBranchingHeuristic<tAdviceSize>::isRSILEnabled() noexcept {
        auto randomNumber = m_rsilHeuristic.getRandomNumberGenerator()();
        return (randomNumber & m_mask) == 0ull;
    }
    
    template<unsigned int tAdviceSize>
    inline void RSILVanishingBranchingHeuristic<tAdviceSize>::updateCallCounter() noexcept {
        assert(m_callCounter > 0);
        --m_callCounter;
        if (m_callCounter == 0) {
            m_mask = (m_mask << 1) | 1ull;
            m_callCounter = m_probHalfLife;
        }
    }
    
    template<unsigned int tAdviceSize>
    __attribute__((always_inline))
    inline Lit RSILVanishingBranchingHeuristic<tAdviceSize>::getAdvice(const TrailType& trail,
                                                                       const TrailLimType& trailLimits,
                                                                       const AssignsType& assigns,
                                                                       const DecisionType& decision) noexcept {
        auto result = isRSILEnabled() ? m_rsilHeuristic.getAdvice(trail, trailLimits, assigns, decision) : lit_Undef;
        updateCallCounter();
        return result;
    }
}

#endif
