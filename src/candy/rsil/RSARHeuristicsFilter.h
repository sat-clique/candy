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

#ifndef X_0395F739_E671_4531_888D_C7654FBF211C_RSARHEURISTICSFILTER_H
#define X_0395F739_E671_4531_888D_C7654FBF211C_RSARHEURISTICSFILTER_H

#include "ImplicitLearningAdvice.h"
#include <rsar/Heuristics.h>
#include <memory>

namespace Candy {
    template<class AdviceEntryType>
    void filterWithRSARHeuristics(std::vector<RefinementHeuristic>& heuristics,
                                  ImplicitLearningAdvice<AdviceEntryType> advice);

    //******* implementation ***********************************************************
    
    namespace RSARHeuristicsFilterImpl {
        inline bool probe(const std::vector<RefinementHeuristic*>& heuristics, Var v, bool isBackbone) {
            for (auto& heuristic : heuristics) {
                if (heuristic->probe(v, isBackbone)) {
                    return true;
                }
            }
            return false;
        }
        
        template<class AdviceEntryType>
        inline void filterWithRSARHeuristics(const std::vector<RefinementHeuristic*>& heuristics,
                                      ImplicitLearningAdvice<AdviceEntryType>& advice,
                                      Var variable) {
            AdviceEntryType& adviceEntry = advice.getAdvice(variable);
            if (adviceEntry.getSize() == 0) {
                return;
            }
            
            if (probe(heuristics, variable, adviceEntry.isBackbone())) {
                adviceEntry.clear();
            }
            else if (adviceEntry.getSize() > 0) {
                for (int i = adviceEntry.getSize()-1; i >= 0; --i) {
                    unsigned int idx = static_cast<unsigned int>(i);
                    
                    if (probe(heuristics, var(adviceEntry.getLiteral(idx)), adviceEntry.isBackbone())) {
                        adviceEntry.removeLiteral(idx);
                    }
                }
            }
        }
    }
    
    template<class AdviceEntryType>
    void filterWithRSARHeuristics(const std::vector<RefinementHeuristic*>& heuristics,
                                  ImplicitLearningAdvice<AdviceEntryType>& advice) {
        
        for (Var v = 0; advice.hasPotentialAdvice(v); ++v) {
            RSARHeuristicsFilterImpl::filterWithRSARHeuristics(heuristics, advice, v);
        }
    }
}

#endif
