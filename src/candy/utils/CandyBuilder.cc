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

#include "candy/utils/CandyBuilder.h"

#include "candy/core/CandySolverInterface.h"

#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"

#include "candy/core/Solver.h"

namespace Candy {

template<class TPropagation, class TLearning, class TBranching> 
CandySolverInterface* CandyBuilder<TPropagation, TLearning, TBranching>::build(CNFProblem& problem) {
    std::cout << "c Building Solver with " << __PRETTY_FUNCTION__ << std::endl;
    return new Solver<TPropagation, TLearning, TBranching>(problem);
}

CandySolverInterface* createSolver(CNFProblem& problem) {
    CandyBuilder<> builder { }; 

    if (SolverOptions::opt_use_lrb) {
        if (ParallelOptions::opt_static_propagate) {
            return builder.branchWithLRB().propagateStaticClauses().build(problem);
        } else if (ParallelOptions::opt_lb_propagate) {
            return builder.branchWithLRB().propagateLowerBounds().build(problem);
        } else if (ParallelOptions::opt_3full_propagate) {
            return builder.branchWithLRB().propagate3Full().build(problem);
        } else if (Stability::opt_prop_by_stability) {
            return builder.branchWithLRB().propagateStable1W().build(problem);
        } else if (ParallelOptions::opt_Xfull_propagate == 3) {
            return builder.branchWithLRB().propagateX3().build(problem);
        } else if (ParallelOptions::opt_Xfull_propagate == 4) {
            return builder.branchWithLRB().propagateX4().build(problem);
        } else if (ParallelOptions::opt_Xfull_propagate == 5) {
            return builder.branchWithLRB().propagateX5().build(problem);
        } else {
            return builder.branchWithLRB().build(problem);
        }
    } 
    else {
        if (ParallelOptions::opt_static_propagate) {
            return builder.propagateStaticClauses().build(problem);
        } else if (ParallelOptions::opt_lb_propagate) {
            return builder.propagateLowerBounds().build(problem);
        } else if (ParallelOptions::opt_3full_propagate) {
            return builder.propagate3Full().build(problem);
        } else if (Stability::opt_prop_by_stability) {
            return builder.propagateStable1W().build(problem);
        } else if (ParallelOptions::opt_Xfull_propagate == 3) {
            return builder.propagateX3().build(problem);
        } else if (ParallelOptions::opt_Xfull_propagate == 4) {
            return builder.propagateX4().build(problem);
        } else if (ParallelOptions::opt_Xfull_propagate == 5) {
            return builder.propagateX5().build(problem);
        } else {
            return builder.build(problem);
        }
    }
    
    return builder.build(problem);
}

}