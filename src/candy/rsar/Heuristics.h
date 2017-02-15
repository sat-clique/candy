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

#ifndef X_5E0BD96A_AF14_4E37_815C_2C72C1D094A5_HEURISTICS_H
#define X_5E0BD96A_AF14_4E37_815C_2C72C1D094A5_HEURISTICS_H

#include <memory>
#include <vector>

namespace Candy {
    class EquivalenceImplications;
    class Backbones;
    class GateAnalyzer;
    
    /**
     * \class RefinementHeuristic
     *
     * \ingroup RS_AbstractionRefinement
     *
     * \brief A variable removal heuristic for abstraction-refinement-based SAT solving.
     *
     */
    class RefinementHeuristic {
    public:
        // TODO: the following three methods should have the noexcept
        // modifier. Alas, gmock does not seem to support mocking
        // noexcept functions.
        
        /**
         * Informs the heuristic that a new refinement step has begun. Needs to be called
         * during initialization and at the beginning of each refinement step.
         */
        virtual void beginRefinementStep() = 0;
        
        /**
         * Adds variables to be removed at the current refinement step to the equivalence
         * representation's variable removal work queue.
         */
        virtual void markRemovals(EquivalenceImplications& equivalence) = 0;
        
        /**
         * Adds variables to be removed at the current refinement step to the backbone
         * representation's variable removal work queue.
         */
        virtual void markRemovals(Backbones& backbones) = 0;
        
        RefinementHeuristic();
        virtual ~RefinementHeuristic();
        RefinementHeuristic(const RefinementHeuristic &other) = delete;
        RefinementHeuristic& operator= (const RefinementHeuristic& other) = delete;
    };
    
    /**
     * \ingroup RS_AbstractionRefinement
     *
     * Creates an instance of InputDepCountRefinementHeuristic.
     *
     * \param analyzer  the gate analyzer providing the gate structure
     * \param config    a list i0, ..., iN of natural numbers. In refinement step X (X <= N),
     *                  all variable outputs depending on more than iX input variables are
     *                  marked for removal. For X > N, all remaining variables are marked for
     *                  removal.
     */
    std::unique_ptr<RefinementHeuristic> createInputDepCountRefinementHeuristic(GateAnalyzer& analyzer,
                                                                                const std::vector<size_t>& config);
}

#endif
