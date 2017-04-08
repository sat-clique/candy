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
            const Conjectures& conjectures;
        };
        
        RSILBranchingHeuristic();
        explicit RSILBranchingHeuristic(const Parameters& params);
        RSILBranchingHeuristic(RSILBranchingHeuristic&& other) = default;
        RSILBranchingHeuristic& operator=(RSILBranchingHeuristic&& other) = default;
        
        Lit pickBranchLit();
        
        inline Lit getAdvice(const TrailType& trail,
                             const TrailLimType& trailLimits,
                             const AssignsType& assigns,
                             const DecisionType& decision);
        
        
        
    private:
        ImplicitLearningAdvice<AdviceEntry<tAdviceSize>> m_advice;
        FastRandomNumberGenerator m_rng;
    };
    
    
    class RSILBranchingHeuristic3 : public RSILBranchingHeuristic<3> {
    public:
        RSILBranchingHeuristic3() = default;
        explicit RSILBranchingHeuristic3(const Parameters& params);
    };
    
    namespace BranchingHeuristicsImpl {
        Var getMaxVar(const Conjectures& conj) {
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
    inline Lit RSILBranchingHeuristic<tAdviceSize>::getAdvice(const TrailType& trail,
                                                              const TrailLimType& trailLimits,
                                                              const AssignsType& assigns,
                                                              const DecisionType& decision) {
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
}

#endif
