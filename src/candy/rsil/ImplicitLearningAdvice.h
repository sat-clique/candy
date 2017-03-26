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

#ifndef X_B0365966_12B4_43E3_96EF_988DF3D415D6_IMPLICITLEARNINGADVICE_H
#define X_B0365966_12B4_43E3_96EF_988DF3D415D6_IMPLICITLEARNINGADVICE_H

#include <core/SolverTypes.h>
#include <randomsimulation/Conjectures.h>

#include <array>
#include <vector>

namespace Candy {
    
    /**
     * \defgroup RS_ImplicitLearning
     */
    
    /**
     * \class ImplicitLearningAdvice
     *
     * \ingroup RS_ImplicitLearning
     *
     * \brief Access-time-optimized equivalence and backbone datastructure for SAT solvers using implicit learning.
     *
     * TODO: more extensive description.
     *
     * The template argument tMaxAdviceSize gives the maximum equivalence conjecture size to be considered;
     * all conjectures of greater size are simply discarded. The value of tMaxAdvice must be greater than
     * 1.
     */
    template<unsigned int tMaxAdviceSize>
    class ImplicitLearningAdvice {
        static_assert(tMaxAdviceSize > 1, "max. advice size tMaxAdviceSize must be larger than 1");
        
    public:
        
        /**
         * \class AdviceLits
         *
         * \ingroup RS_ImplicitLearning
         *
         * \brief AdviceLits stores an array of tMaxAdviceSize-1 literals.
         */
        using AdviceLits = std::array<Lit, tMaxAdviceSize-1>;

        /**
         * \class AdviceEntry
         *
         * \ingroup RS_ImplicitLearning
         *
         * \brief Advice for implicit learning heuristics, regarding a single variable.
         *
         * AdviceEntry is responsible for providing advice for implicit learning heuristics,
         * regarding a single fixed variable \a v. \p m_lits is an array of literals containing
         * at least \p m_size valid entries. If \p m_isBackbone is false, \p m_lits contains
         * the literals conjected to be equivalent to the literal \a l with variable \a v and
         * positive sign. If \p m_isBackbone is true, \p m_lits contains exactly the literal
         * \a b conjected to belong to the backbone with the variable of \a b being \a v.
         */
        struct AdviceEntry {
            unsigned int m_size : 31;
            unsigned int m_isBackbone : 1;
            
            AdviceLits m_lits;
            
            void addLiteral(Lit literal) {
                assert((m_size+1) < tMaxAdviceSize);
                m_lits[m_size] = literal;
                ++m_size;
            }
        };
        
        /**
         * \ingroup RS_ImplicitLearning
         *
         * Constructs an instance \a i of ImplicitLearningAdvice.
         *
         * \param conjectures   The set of equivalence/backbone conjectures which should
         *                      be represented by \i. Note that equivalence conjectures of
         *                      size greater than tMaxAdviceSize are discarded.
         *
         * \param maxVar        A variable at least as great as the greatest variable occuring
         *                      in \p conjectures.
         */
        explicit ImplicitLearningAdvice(const Conjectures& conjectures, Var maxVar);
        
        /**
         * \ingroup RS_ImplicitLearning
         *
         * Retrieves implicit learning heuristics advice for the variable \p v .
         *
         * \param v             A variable such that \a hasPotentialAdvice(v) evaluates to true.
         */
        const AdviceEntry& getAdvice(Var v) const noexcept;
        
        /**
         * \ingroup RS_ImplicitLearning
         *
         * Returns true iff \p getAdvice() may be called for \p v .
         */
        bool hasPotentialAdvice(Var v) const noexcept;
        
        ImplicitLearningAdvice(const ImplicitLearningAdvice& other) = delete;
        ImplicitLearningAdvice& operator=(const ImplicitLearningAdvice& other) = delete;
        
    private:
        void addEquivalenceConjecture(const EquivalenceConjecture& conj);
        void addBackboneConjecture(const BackboneConjecture& conj);
        
        std::vector<AdviceEntry> m_advice;
    };
    
    //******* ImplicitLearningAdvice implementation *************************************
    
    template<unsigned int tMaxAdviceSize>
    inline const typename ImplicitLearningAdvice<tMaxAdviceSize>::AdviceEntry&
    ImplicitLearningAdvice<tMaxAdviceSize>::getAdvice(Var v) const noexcept {
        assert(v < m_advice.size());
        return m_advice[v];
    }
    
    template<unsigned int tMaxAdviceSize>
    inline void ImplicitLearningAdvice<tMaxAdviceSize>::addEquivalenceConjecture(const Candy::EquivalenceConjecture &conjecture) {
        for (auto& keyLiteral : conjecture) {
            Var keyVar = var(keyLiteral);
            bool keySign = sign(keyLiteral);
            
            m_advice[keyVar].m_isBackbone = false;
            
            for (auto& entryLiteral : conjecture) {
                if (keyLiteral != entryLiteral) {
                    m_advice[keyVar].addLiteral(keySign ? entryLiteral : ~entryLiteral);
                }
            }
        }
    }
    
    template<unsigned int tMaxAdviceSize>
    inline void ImplicitLearningAdvice<tMaxAdviceSize>::addBackboneConjecture(const Candy::BackboneConjecture &conjecture) {
        Var key = var(conjecture.getLit());
        m_advice[key].m_isBackbone = true;
        m_advice[key].addLiteral(conjecture.getLit());
    }
    
    
    template<unsigned int tMaxAdviceSize>
    ImplicitLearningAdvice<tMaxAdviceSize>::ImplicitLearningAdvice(const Conjectures& conjectures, Var maxVar) {
        m_advice.resize(maxVar+1);
        
        for (auto& conjecture : conjectures.getEquivalences()) {
            if (conjecture.size() <= tMaxAdviceSize) {
                addEquivalenceConjecture(conjecture);
            }
        }
        
        for (auto& conjecture : conjectures.getBackbones()) {
            addBackboneConjecture(conjecture);
        }
    }
    
    template<unsigned int tMaxAdviceSize>
    bool ImplicitLearningAdvice<tMaxAdviceSize>::hasPotentialAdvice(Var v) const noexcept {
        assert(v > 0);
        return static_cast<unsigned int>(v) < m_advice.size();
    }
}

#endif
