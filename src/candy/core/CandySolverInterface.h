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

#ifndef SRC_CANDY_CORE_CANDYSOLVERINTERFACE_H_
#define SRC_CANDY_CORE_CANDYSOLVERINTERFACE_H_

#include "candy/core/SolverTypes.h"

#include <vector>

namespace Candy {

class ClauseAllocator;
class Statistics;
class BranchingDiversificationInterface;
class ClauseDatabase;
class Trail;
class CandySolverResult;
class CNFProblem;

class CandySolverInterface {
public:
	virtual ~CandySolverInterface() {};

	virtual ClauseAllocator* setupGlobalAllocator() = 0;
	virtual void clear() = 0;
    virtual void init(const CNFProblem& problem, ClauseAllocator* allocator = nullptr, bool lemma = true) = 0;
	
	virtual lbool solve() = 0;
	virtual void printStats() = 0;
	virtual unsigned int nVars() const = 0;
	virtual unsigned int nClauses() const = 0;
	virtual unsigned int nConflicts() const = 0;

	virtual BranchingDiversificationInterface* getBranchingUnit() = 0;

	virtual ClauseDatabase& getClauseDatabase() = 0;
	virtual Trail& getAssignment() = 0; 

	virtual CandySolverResult& getCandySolverResult() = 0;

	virtual void setTermCallback(void* state, int (*termCallback)(void* state)) = 0;
	virtual void setLearntCallback(void* state, int max_length, void (*learntCallback)(void* state, int* clause)) = 0;

};

}
#endif /* SRC_CANDY_CORE_CANDYSOLVERINTERFACE_H_ */
