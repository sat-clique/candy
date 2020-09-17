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

template<class TPropagate, class TLearning, class TBranching> 
CandySolverInterface* CandyBuilder<TPropagate, TLearning, TBranching>::build() {
    return new Solver<TPropagate, TLearning, TBranching>();
}

CandySolverInterface* createSolver(bool staticPropagate, bool lrb) {
    CandyBuilder<> builder { }; 

    if (lrb) {
        if (staticPropagate) {
            std::cout << "c Building Solver of Type Solver<StaticPropagate, ConflictAnalysis, LRB>" << std::endl;
            return builder.branchWithLRB().propagateStaticClauses().build();
        } else {
            std::cout << "Building Solver of Type Solver<Propagate, ConflictAnalysis, LRB>" << std::endl;
            return builder.branchWithLRB().build();
        }
    } 
    else {
        if (staticPropagate) {
            std::cout << "c Building Solver of Type Solver<StaticPropagate, ConflictAnalysis, VSIDS>" << std::endl;
            return builder.propagateStaticClauses().build();
        } else {
            std::cout << "c Building Solver of Type Solver<Propagate, ConflictAnalysis, VSIDS>" << std::endl;
            return builder.build();
        }
    }
    std::cout << "c Warning! Configuration Not Found. Building Solver of Type Solver<Propagate, ConflictAnalysis, VSIDS>" << std::endl;
    return builder.build();
}

}