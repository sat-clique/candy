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

#ifndef X_70571638_C307_4548_8A51_938865519EE0_HEURISTICSMOCK_H
#define X_70571638_C307_4548_8A51_938865519EE0_HEURISTICSMOCK_H

#include <gmock/gmock.h>
#include <core/SolverTypes.h>
#include <rsar/Heuristics.h>

#include <map>

namespace Candy {
    /**
     * \class MockHeuristic
     *
     * \ingroup RS_AbstractionRefinement_Tests
     *
     * \brief A gmock-mockable subclass of RefinementHeuristic.
     */
    class MockHeuristic : public RefinementHeuristic {
    public:
        MOCK_METHOD0(beginRefinementStep, void ());
        MOCK_METHOD1(markRemovals, void(EquivalenceImplications&));
        MOCK_METHOD1(markRemovals, void(Backbones& backbones));
        MOCK_METHOD2(probe, bool(Var, bool));
    };
    
    /**
     * \class FakeHeuristic
     *
     * \ingroup RS_AbstractionRefinement_Tests
     *
     * \brief A configurable RefinementHeuristic implementation for testing.
     */
    class FakeHeuristic : public RefinementHeuristic {
    public:
        void beginRefinementStep() override;
        void markRemovals(EquivalenceImplications&) override;
        void markRemovals(Backbones& backbones) override;
        
        /** Causes the fake heuristic to mark the given variables deactivated after
         * the given amount of beginRefinementStep() invocations. */
        void inStepNRemove(int step, const std::vector<Var>& variables) noexcept;
        
        FakeHeuristic();
        virtual ~FakeHeuristic();
        
        
    private:
        std::map<int, std::vector<Var>> m_removals;
        int m_step;
    };
}

#endif
