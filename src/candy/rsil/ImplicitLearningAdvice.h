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
     *
     * The template argument tMaxAdviceSize gives the maximum amount of literals which
     * may be represented by the AdviceEntry instance and is stored in AdviceEntry::maxSize.
     * (This includes the literal or variable by which the instance is addressed. Therefore,
     * at most tMaxAdviceSize-1 literals may be stored directly in the AdviceEntry instance.)
     */
    template<unsigned int tMaxAdviceSize>
    class AdviceEntry {
    public:
        static_assert(tMaxAdviceSize > 1, "max. advice size tMaxAdviceSize must be larger than 1");
        
        /**
         * \class AdviceLits
         *
         * \ingroup RS_ImplicitLearning
         *
         * \brief AdviceLits stores an array of tMaxAdviceSize-1 literals.
         */
        using AdviceLits = std::array<Lit, tMaxAdviceSize-1>;
        
        /** equals the tMaxAdviceSize parameter */
        static const unsigned int maxSize = tMaxAdviceSize;
        
        /**
         * \returns the amount of literals stored in this entry.
         */
        inline unsigned int getSize() const noexcept {
            return m_size;
        }
        
        /**
         * \returns \p true iff this entry represents a backbone literal.
         */
        inline unsigned int isBackbone() const noexcept {
            return m_isBackbone;
        }
        
        /**
         * \param index   a literal index smaller than the value of getSize().
         * \returns the literal at index \p index .
         */
        inline Lit getLiteral(unsigned int index) const noexcept {
            assert(index < m_size);
            return m_lits[index];
        }
        
        /**
         * Removes the literal at index i, replacing it with the last literal if
         * applicable.
         *
         * \param index   a literal index smaller than the value of getSize().
         */
        inline void removeLiteral(unsigned int index) noexcept {
            assert(index < m_size);
            if (m_size > 1 && index+1 != m_size) {
                m_lits[index] = m_lits[m_size-1];
            }
            m_size--;
        }
        
        /**
         * Removes all literals from the advice entry.
         */
        inline void clear() noexcept {
            m_size = 0;
        }
        
        /**
         * Adds a literal to the advice entry.
         *
         * \param literal   a literal to be added to this entry.
         */
        inline void addLiteral(Lit literal) noexcept {
            assert((m_size+1) < tMaxAdviceSize);
            m_lits[m_size] = literal;
            ++m_size;
        }
        
        /**
         * Marks the advice entry as a backbone literal representation.
         *
         * \param isBackbone    true iff this entry represents a backbone literal.
         */
        inline void setBackbone(bool isBackbone) noexcept {
            m_isBackbone = isBackbone;
        }
        
    private:
        unsigned int m_size : 31;
        unsigned int m_isBackbone : 1;
        
        AdviceLits m_lits;
    };
    
    /**
     * \class BudgetAdviceEntry
     *
     * \ingroup RS_ImplicitLearning
     *
     * \brief Extension of AdviceEntry with a budget associated with each literal.
     *
     * The template argument tMaxAdviceSize gives the maximum amount of literals which
     * may be stored in the AdviceEntry instance and is stored in AdviceEntry::maxSize.
     */
    template<unsigned int tMaxAdviceSize>
    class BudgetAdviceEntry {
        static_assert(tMaxAdviceSize > 1, "max. advice size tMaxAdviceSize must be larger than 1");
        
    public:
        using AdviceBudgets = std::array<int, tMaxAdviceSize-1>;
        
        /** equals the tMaxAdviceSize parameter */
        static const unsigned int maxSize = tMaxAdviceSize;
        
        /**
         * \returns the amount of literals stored in this entry.
         */
        inline unsigned int getSize() const noexcept {
            return m_advice.getSize();
        }
        
        /**
         * \returns \p true iff this entry represents a backbone literal.
         */
        inline unsigned int isBackbone() const noexcept {
            return m_advice.isBackbone();
        }
        
        /**
         * \param index   a literal index smaller than the value of getSize().
         * \returns the literal at index \p index .
         */
        inline Lit getLiteral(unsigned int index) const noexcept {
            return m_advice.getLiteral(index);
        }
        
        /**
         * Removes the literal at index i, replacing it with the last literal if
         * applicable.
         *
         * \param index   a literal index smaller than the value of getSize().
         */
        inline void removeLiteral(unsigned int index) noexcept {
            if (getSize() > 1 && index+1 != getSize()) {
                m_budgets[index] = m_budgets[getSize()-1];
            }
            m_advice.removeLiteral(index);
        }
        
        /**
         * Removes all literals from the advice entry.
         */
        inline void clear() noexcept {
            m_advice.clear();
        }
        
        /**
         * Adds a literal to the advice entry.
         *
         * \param literal   a literal to be added to this entry.
         */
        inline void addLiteral(Lit literal) noexcept {
            m_advice.addLiteral(literal);
        }
        
        /**
         * Marks the advice entry as a backbone literal representation.
         *
         * \param isBackbone    true iff this entry represents a backbone literal.
         */
        inline void setBackbone(bool isBackbone) noexcept {
            m_advice.setBackbone(isBackbone);
        }
        
        
        /**
         * Gets the budget for the literal at the given index.
         *
         * \param index     A literal index, smaller than the value of \p getSize() .
         * \returns the budget assigned for the the literal at index \p index .
         */
        inline int getBudget(unsigned int index) const noexcept {
            assert(index < this->getSize());
            return m_budgets[index];
        }
        
        /**
         * Sets the budget for the literal at the given index.
         *
         * \param index     A literal index, smaller than the value of \p getSize() .
         */
        inline void setBudget(unsigned int index, unsigned int budget) noexcept {
            assert(index+1 < tMaxAdviceSize);
            m_budgets[index] = budget;
        }
        
    private:
        AdviceEntry<tMaxAdviceSize> m_advice;
        AdviceBudgets m_budgets;
    };
    
    
    /**
     * \class ImplicitLearningAdvice
     *
     * \ingroup RS_ImplicitLearning
     *
     * \brief Access-time-optimized equivalence and backbone datastructure for SAT solvers using implicit learning.
     *
     * TODO: more extensive description.
     *
     *
     * AdviceEntryType must be AdviceEntry or a subtype of AdviceEntry.
     * The value AdviceEntryType::maxSize gives the maximum equivalence conjecture size to be considered;
     * all conjectures of greater size are simply discarded. The value of tMaxAdvice must be greater than
     * 1.
     */
    template<class AdviceEntryType>
    class ImplicitLearningAdvice {
        static_assert(AdviceEntryType::maxSize > 0, "max. advice size must be larger than zero");
        
    public:
        
        /**
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
         * Retrieves implicit learning heuristics advice for the variable \p v .
         *
         * Note: for the sake of efficient implementation, this method does not return a const reference.
         *
         * \param v             A variable such that \a hasPotentialAdvice(v) evaluates to true.
         */
        inline AdviceEntryType& getAdvice(Var v) noexcept;
        
        /**
         * Returns true iff \p getAdvice() may be called for \p v .
         */
        inline bool hasPotentialAdvice(Var v) const noexcept;
        
        ImplicitLearningAdvice(const ImplicitLearningAdvice& other) = delete;
        ImplicitLearningAdvice& operator=(const ImplicitLearningAdvice& other) = delete;
        
    private:
        void addEquivalenceConjecture(const EquivalenceConjecture& conj);
        void addBackboneConjecture(const BackboneConjecture& conj);
        
        std::vector<AdviceEntryType> m_advice;
    };
    
    //******* ImplicitLearningAdvice implementation *************************************
    
    template<class AdviceEntryType>
    AdviceEntryType&
    ImplicitLearningAdvice<AdviceEntryType>::getAdvice(Var v) noexcept {
        assert(v < m_advice.size());
        return m_advice[v];
    }
    
    template<class AdviceEntryType>
    void ImplicitLearningAdvice<AdviceEntryType>::addEquivalenceConjecture(const Candy::EquivalenceConjecture &conjecture) {
        for (auto& keyLiteral : conjecture) {
            Var keyVar = var(keyLiteral);
            bool keySign = sign(keyLiteral);
            
            m_advice[keyVar].setBackbone(false);
            
            for (auto& entryLiteral : conjecture) {
                if (keyLiteral != entryLiteral) {
                    m_advice[keyVar].addLiteral(keySign ? entryLiteral : ~entryLiteral);
                }
            }
        }
    }
    
    template<class AdviceEntryType>
    void ImplicitLearningAdvice<AdviceEntryType>::addBackboneConjecture(const Candy::BackboneConjecture &conjecture) {
        Var key = var(conjecture.getLit());
        m_advice[key].setBackbone(true);
        m_advice[key].addLiteral(conjecture.getLit());
    }
    
    
    template<class AdviceEntryType>
    ImplicitLearningAdvice<AdviceEntryType>::ImplicitLearningAdvice(const Conjectures& conjectures, Var maxVar) {
        m_advice.resize(maxVar+1);
        
        for (auto& conjecture : conjectures.getEquivalences()) {
            if (conjecture.size() <= AdviceEntryType::maxSize) {
                addEquivalenceConjecture(conjecture);
            }
        }
        
        for (auto& conjecture : conjectures.getBackbones()) {
            addBackboneConjecture(conjecture);
        }
    }
    
    template<class AdviceEntryType>
    bool ImplicitLearningAdvice<AdviceEntryType>::hasPotentialAdvice(Var v) const noexcept {
        assert(v >= 0);
        return static_cast<unsigned int>(v) < m_advice.size();
    }
}

#endif
