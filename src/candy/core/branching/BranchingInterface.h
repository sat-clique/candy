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

    // BranchingInterface(BranchingInterface&& other) : BranchingMethod(other) {
        
	// }

    // BranchingInterface& operator=(BranchingInterface&& other) {

    // }

    void notify_conflict(AnalysisResult ana, Trail& trail, unsigned int learnt_lbd) {
        static_cast<BranchingMethod *>(this)->notify_conflict(ana, trail, learnt_lbd);
    }

    void notify_backtracked(std::vector<Lit> lits) {
        static_cast<BranchingMethod *>(this)->notify_backtracked(lits);
    }

    void notify_restarted(Trail& trail) {
        static_cast<BranchingMethod *>(this)->notify_restarted(trail);
    }

    inline Lit pickBranchLit(Trail& trail) {
        return static_cast<BranchingMethod *>(this)->pickBranchLit(trail);
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