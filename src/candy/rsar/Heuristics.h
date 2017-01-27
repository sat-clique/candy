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

#ifndef X_5E0BD96A_AF14_4E37_815C_2C72C1D094A5_HEURISTICS_H
#define X_5E0BD96A_AF14_4E37_815C_2C72C1D094A5_HEURISTICS_H

namespace Candy {
    class EquivalenceImplications;
    class Backbones;
    
    class RefinementHeuristic {
    public:
        virtual void beginRefinementStep() = 0;
        virtual void markRemovals(EquivalenceImplications& equivalence) = 0;
        virtual void markRemovals(Backbones& backbones) = 0;
        
        RefinementHeuristic();
        virtual ~RefinementHeuristic();
        RefinementHeuristic(const RefinementHeuristic &other) = delete;
        RefinementHeuristic& operator= (const RefinementHeuristic& other) = delete;
    };
}

#endif
