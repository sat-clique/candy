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
#include "candy/core/CNFProblem.h"

#include <vector>

namespace Candy {

class BranchingDiversificationInterface;
class ClauseAllocator;

class CandySolverInterface {
public:
	virtual ~CandySolverInterface() {};

	virtual BranchingDiversificationInterface* accessBranchingInterface() = 0;

	virtual void enablePreprocessing() = 0;
    virtual void disablePreprocessing() = 0;

    virtual Var newVar() = 0;

	virtual ClauseAllocator* setupGlobalAllocator() = 0;
    virtual void init(const CNFProblem& problem, ClauseAllocator* allocator = nullptr) = 0;
	virtual bool addClause(const Cl& clause) = 0;

    virtual void unit_resolution() = 0; // remove satisfied clauses and remove false literals from clauses	
    virtual void eliminate() = 0;  // Perform variable elimination based simplification.
	
    virtual bool isEliminated(Var v) const = 0;
    virtual void setFrozen(Var v, bool freeze) = 0;

	virtual lbool solve() = 0;
	virtual lbool solve(std::initializer_list<Lit> assumps) = 0;
	virtual lbool solve(const std::vector<Lit>& assumps) = 0;

	virtual void setConfBudget(uint64_t x) = 0;
	virtual void setPropBudget(uint64_t x) = 0;
	virtual void setInterrupt(bool value) = 0;
	virtual void budgetOff() = 0;
	virtual void setLearntCallback(void* state, int max_length, void (*learntCallback)(void* state, int* clause)) = 0;

	virtual void printDIMACS() = 0;

	// The value of a variable in the last model. The last call to solve must have been satisfiable.
	virtual lbool modelValue(Var x) const = 0;
	virtual lbool modelValue(Lit p) const = 0;
    virtual Cl getModel() = 0;

	// true means solver is in a conflicting state
	virtual bool isInConflictingState() const = 0;
	virtual std::vector<Lit>& getConflict() = 0;

	virtual size_t nClauses() const = 0;
    virtual size_t nVars() const = 0;
	virtual size_t nConflicts() const = 0;
	virtual size_t nPropagations() const = 0;
};

}
#endif /* SRC_CANDY_CORE_CANDYSOLVERINTERFACE_H_ */
