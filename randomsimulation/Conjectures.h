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

#ifndef _AAE7E9E3_CD0B_407F_8794_C5A8B42B678A_CONJECTURES_H
#define _AAE7E9E3_CD0B_407F_8794_C5A8B42B678A_CONJECTURES_H

#include <core/SolverTypes.h>

namespace randsim {
    class EquivalenceConjecture {
    public:
        void addLit(Glucose::Lit lit);
        const std::vector<Glucose::Lit> getLits() const;
        
    private:
        std::vector<Glucose::Lit> m_lits {};
    };
    
    class BackboneConjecture {
    public:
        BackboneConjecture(Glucose::Lit lit);
        Glucose::Lit getLit();
        
    private:
        Glucose::Lit m_lit;
    };
    
    class Conjectures {
    public:
        const std::vector<EquivalenceConjecture> &getEquivalences() const;
        const std::vector<BackboneConjecture> &getBackbones() const;
        
        void addEquivalence(EquivalenceConjecture &conj);
        void addBackbone(BackboneConjecture &conj);
        
    private:
        std::vector<EquivalenceConjecture> m_equivalences {};
        std::vector<BackboneConjecture> m_backbones {};
    };
}


#endif
