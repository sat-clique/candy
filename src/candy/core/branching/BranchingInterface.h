/*
 * BranchingInterface.h
 *
 *  Created on: 30.07.2018
 *      Author: Markus Iser
 */

#ifndef SRC_CANDY_CORE_BRANCHING_INTERFACE_H_
#define SRC_CANDY_CORE_BRANCHING_INTERFACE_H_

#include <vector>

#include "candy/core/Trail.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/ConflictAnalysis.h"

namespace Candy {

template<class BranchingMethod>
class BranchingInterface {

protected:

    BranchingInterface() {
    }

public:

    void setPolarity(Var v, bool sign) {
        static_cast<BranchingMethod *>(this)->setPolarity(v, sign);
    }

    Lit getLastDecision() {
        return static_cast<BranchingMethod *>(this)->getLastDecision();
    }

    void notify_conflict() {
        static_cast<BranchingMethod *>(this)->notify_conflict();
    }

    void notify_backtracked() {
        static_cast<BranchingMethod *>(this)->notify_backtracked();
    }

    void notify_restarted() {
        static_cast<BranchingMethod *>(this)->notify_restarted();
    }

    inline Lit pickBranchLit() {
        return static_cast<BranchingMethod *>(this)->pickBranchLit();
    }

    void setDecisionVar(Var v, bool b) {
        static_cast<BranchingMethod *>(this)->setDecisionVar(v, b);
    }

    bool isDecisionVar(Var v) {
        return static_cast<BranchingMethod *>(this)->isDecisionVar(v);
    }

    void grow() {
        static_cast<BranchingMethod *>(this)->grow();
    }

    void grow(size_t size) {
        static_cast<BranchingMethod *>(this)->grow(size);
    }

    void initFrom(const CNFProblem& problem) {
        static_cast<BranchingMethod *>(this)->initFrom(problem);
    }

};

}

#endif /* SRC_CANDY_CORE_BRANCHING_INTERFACE_H_ */