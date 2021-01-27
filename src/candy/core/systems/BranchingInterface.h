/*************************************************************************************************
Candy -- Copyright (c) 2015-2020, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef BRANCHING_INTERFACE_H_
#define BRANCHING_INTERFACE_H_

#include "candy/core/SolverTypes.h"

namespace Candy {

class BranchingInterface {
public:
    virtual void clear() = 0;
    virtual void init(const CNFProblem& problem) = 0;
    virtual void reset() = 0;
    virtual void process_conflict() = 0;
    virtual Lit pickBranchLit() = 0;

    // diversification purpose
    virtual void setPolarity(Var v, bool sign) = 0;
    virtual Lit getLastDecision() = 0;
};

}

#endif