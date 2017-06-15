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

#ifndef X_FF52A936_70FC_4DD4_A1F8_EC9D6425EB9C_TESTUTILS_H
#define X_FF52A936_70FC_4DD4_A1F8_EC9D6425EB9C_TESTUTILS_H

#include <candy/core/SolverTypes.h>
#include <candy/rsil/ImplicitLearningAdvice.h>

namespace Candy {
    template<class AdviceEntryType>
    bool isEquivalenceAdvised(ImplicitLearningAdvice<AdviceEntryType>& advice, Lit key, Lit equivalentLiteral) {
        auto& adviceEntry = advice.getAdvice(var(key));
        
        if (adviceEntry.isBackbone()) {
            return false;
        }
        
        Lit searchedLit = sign(key) ? equivalentLiteral : ~equivalentLiteral;
        for (size_t i = 0; i < adviceEntry.getSize(); ++i) {
            if (adviceEntry.getLiteral(i) == searchedLit) {
                return true;
            }
        }
        
        return false;
    }
    
    template<class AdviceEntryType>
    bool isBackboneAdvised(ImplicitLearningAdvice<AdviceEntryType>& advice, Lit backboneLiteral) {
        auto& adviceEntry = advice.getAdvice(var(backboneLiteral));
        return adviceEntry.isBackbone()
        && adviceEntry.getSize() == 1
        && adviceEntry.getLiteral(0) == backboneLiteral;
    }
}

#endif
