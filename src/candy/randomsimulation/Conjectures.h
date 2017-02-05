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

#ifndef X_AAE7E9E3_CD0B_407F_8794_C5A8B42B678A_CONJECTURES_H
#define X_AAE7E9E3_CD0B_407F_8794_C5A8B42B678A_CONJECTURES_H

#include <core/SolverTypes.h>

#include <memory>

namespace Candy {
    /**
     * \class EquivalenceConjecture
     *
     * \ingroup RandomSimulation
     *
     * \brief A set of literals conjected to be equivalent.
     */
    class EquivalenceConjecture {
    public:
        typedef std::vector<Glucose::Lit>::const_iterator const_iterator;
        typedef std::vector<Glucose::Lit>::size_type size_type;
        
        /**
         * Adds a literal to the conjecture.
         */
        void addLit(Glucose::Lit lit);
        
        const_iterator begin() const;
        const_iterator end() const;
        size_type size() const;
        Lit at(size_type index) const;
        
        EquivalenceConjecture();
    private:
        std::vector<Glucose::Lit> m_lits;
    };
    
    /**
     * \class BackboneConjecture
     *
     * \ingroup RandomSimulation
     *
     * \brief A literal conjected to be a backbone literal, i.e.
     *   always being evaluated to true.
     */
    class BackboneConjecture {
    public:
        explicit BackboneConjecture(Glucose::Lit lit);
        
        /**
         * Retrieves the literal conjected to belong to the backbone.
         */
        Glucose::Lit getLit() const;
        
    private:
        Glucose::Lit m_lit;
    };
    
    /**
     * \class Conjectures
     *
     * \ingroup RandomSimulation
     *
     * \brief A collection of simple conjectures about literals.
     */
    class Conjectures {
    public:
        /**
         * Retrieves the equivalence conjectures.
         */
        const std::vector<EquivalenceConjecture> &getEquivalences() const;
        
        /**
         * Retrieves the backbone conjectures.
         */
        const std::vector<BackboneConjecture> &getBackbones() const;
        
        /**
         * Adds an equivalence conjecture (by copying).
         */
        void addEquivalence(const EquivalenceConjecture &conj);
        
        /**
         * Adds a backbone conjecture (by copying).
         */
        void addBackbone(const BackboneConjecture &conj);
        
    private:
        std::vector<EquivalenceConjecture> m_equivalences {};
        std::vector<BackboneConjecture> m_backbones {};
    };
    
    /**
     * \class ConjectureFilter
     *
     * \ingroup RandomSimulation
     *
     * \brief A filter for conjectures.
     */
    class ConjectureFilter {
    public:
        virtual Conjectures apply(const Conjectures& c) const = 0;
        
        ConjectureFilter();
        virtual ~ConjectureFilter();
        ConjectureFilter(const ConjectureFilter& other) = delete;
        ConjectureFilter& operator= (const ConjectureFilter& other) = delete;
    };
    
    /**
     * \ingroup RandomSimulation
     *
     * Creates a ConjectureFilter removing equivalence conjectures larger than the
     * given size.
     */
    std::unique_ptr<ConjectureFilter> createSizeConjectureFilter(size_t maxEquivSize);
    
    /**
     * \ingroup RandomSimulation
     *
     * Creates a ConjectureFilter removing backbones from the conjecture set.
     */
    std::unique_ptr<ConjectureFilter> createBackboneRemovalConjectureFilter();
}


#endif
