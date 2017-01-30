/* Copyright (c) 2017 Felix Kutzner
 
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

// TODO: documentation

#include "Conjectures.h"

namespace Candy {
    EquivalenceConjecture::EquivalenceConjecture() : m_lits() {
        
    }
    
    void EquivalenceConjecture::addLit(Glucose::Lit lit) {
        m_lits.push_back(lit);
    }
    
    EquivalenceConjecture::const_iterator EquivalenceConjecture::begin() const {
        return m_lits.begin();
    }
    
    EquivalenceConjecture::const_iterator EquivalenceConjecture::end() const {
        return m_lits.end();
    }
    
    EquivalenceConjecture::size_type EquivalenceConjecture::size() const {
        return m_lits.size();
    }
    
    Lit EquivalenceConjecture::at(EquivalenceConjecture::size_type index) const {
        return m_lits[index];
    }
    
    BackboneConjecture::BackboneConjecture(Glucose::Lit lit) : m_lit(lit) {
        
    }
    
    Glucose::Lit BackboneConjecture::getLit() const {
        return m_lit;
    }
    
    void Conjectures::addEquivalence(const EquivalenceConjecture &conj) {
        m_equivalences.push_back(conj);
    }
    
    void Conjectures::addBackbone(const BackboneConjecture &conj) {
        m_backbones.push_back(conj);
    }
    
    const std::vector<EquivalenceConjecture> &Conjectures::getEquivalences() const {
        return m_equivalences;
    }
    
    const std::vector<BackboneConjecture> &Conjectures::getBackbones() const {
        return m_backbones;
    }
}
