/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#ifndef CANDY_BUILDER_H_
#define CANDY_BUILDER_H_

#include "candy/systems/propagate/Propagate.h"
#include "candy/systems/propagate/StaticPropagate.h"
#include "candy/systems/learning/ConflictAnalysis.h"
#include "candy/systems/branching/LRB.h"
#include "candy/systems/branching/VSIDS.h"

namespace Candy {

class CandySolverInterface; 

template<class TPropagate = Propagate, class TLearning = ConflictAnalysis, class TBranching = VSIDS> 
class CandyBuilder { 
public:
    constexpr CandyBuilder() { }

    constexpr auto branchWithVSIDS() const -> CandyBuilder<TPropagate, TLearning, VSIDS> {
        return CandyBuilder<TPropagate, TLearning, VSIDS>();
    }

    constexpr auto branchWithLRB() const -> CandyBuilder<TPropagate, TLearning, LRB> {
        return CandyBuilder<TPropagate, TLearning, LRB>();
    }

    constexpr auto propagateStaticClauses() const -> CandyBuilder<StaticPropagate, TLearning, TBranching> { 
        return CandyBuilder<StaticPropagate, TLearning, TBranching>();
    }

    CandySolverInterface* build();

};

CandySolverInterface* createSolver(bool staticPropagate = false, bool lrb = false);

}

#endif