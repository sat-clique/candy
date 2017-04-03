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

#include "BranchingHeuristics.h"
#include <candy/core/Solver.h>

namespace Candy {
    RSILBranchingHeuristic::RSILBranchingHeuristic() {
    }
    
    RSILBranchingHeuristic::RSILBranchingHeuristic(const RSILBranchingHeuristic::Parameters& params) {
        (void) params;
    }
    
    Lit RSILBranchingHeuristic::pickBranchLit() {
        return mkLit(0); // TODO: implementation
    }
    
    template <>
    Lit Solver<RSILBranchingHeuristic>::pickBranchLit() {
        
        // TODO: the following is a duplication of the default pickBranchLit.
        // Having been introduced to minimize the merging overhead, it will be
        // replaced just before the next pull request.
        
        Var next = var_Undef;
        
        // Activity based decision:
        while (next == var_Undef || value(next) != l_Undef || !decision[next]) {
            if (order_heap.empty()) {
                next = var_Undef;
                break;
            } else {
                next = order_heap.removeMin();
            }
        }
        
        return next == var_Undef ? lit_Undef : mkLit(next, polarity[next]);
    }
}
