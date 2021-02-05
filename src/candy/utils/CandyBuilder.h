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

#include "candy/core/systems/Propagation2WL.h"
#include "candy/core/systems/Propagation2WLStatic.h"
#include "candy/core/systems/Propagation2WL3Full.h"
#include "candy/core/systems/PropagationLB.h"
#include "candy/core/systems/Learning1UIP.h"
#include "candy/core/systems/BranchingVSIDS.h"
#include "candy/core/systems/BranchingLRB.h"

namespace Candy {

class CandySolverInterface; 

template<class TPropagation = Propagation2WL, class TLearning = Learning1UIP, class TBranching = BranchingVSIDS> 
class CandyBuilder { 
public:
    constexpr CandyBuilder() { }

    constexpr auto branchWithVSIDS() const -> CandyBuilder<TPropagation, TLearning, BranchingVSIDS> {
        return CandyBuilder<TPropagation, TLearning, BranchingVSIDS>();
    }

    constexpr auto branchWithLRB() const -> CandyBuilder<TPropagation, TLearning, BranchingLRB> {
        return CandyBuilder<TPropagation, TLearning, BranchingLRB>();
    }

    constexpr auto propagateStaticClauses() const -> CandyBuilder<Propagation2WLStatic, TLearning, TBranching> { 
        return CandyBuilder<Propagation2WLStatic, TLearning, TBranching>();
    }

    constexpr auto propagateLowerBounds() const -> CandyBuilder<PropagationLB, TLearning, TBranching> { 
        return CandyBuilder<PropagationLB, TLearning, TBranching>();
    }

    constexpr auto propagate3Full() const -> CandyBuilder<Propagation2WL3Full, TLearning, TBranching> { 
        return CandyBuilder<Propagation2WL3Full, TLearning, TBranching>();
    }

    CandySolverInterface* build(CNFProblem& problem);

};

CandySolverInterface* createSolver(CNFProblem& problem);

}

#endif