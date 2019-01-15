#ifndef CANDY_BUILDER_H_
#define CANDY_BUILDER_H_

#include "candy/core/Solver.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/clauses/ClauseAllocator.h"
#include "candy/core/Trail.h"
#include "candy/core/propagate/Propagate.h"
#include "candy/core/propagate/StaticPropagate.h"
#include "candy/core/learning/ConflictAnalysis.h"
#include "candy/core/branching/LRB.h"
#include "candy/core/branching/VSIDS.h"
#include "candy/rsil/BranchingHeuristics.h"

namespace Candy {

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

    constexpr auto branchWithRSILUnrestricted3() const -> CandyBuilder<TPropagate, TLearning, RSILBranchingHeuristic3> {
        return CandyBuilder<TPropagate, TLearning, RSILBranchingHeuristic3>();
    }

    constexpr auto branchWithRSILBudgeted3() const -> CandyBuilder<TPropagate, TLearning, RSILVanishingBranchingHeuristic3> {
        return CandyBuilder<TPropagate, TLearning, RSILVanishingBranchingHeuristic3>();
    }

    constexpr auto branchWithRSILVanishing3() const -> CandyBuilder<TPropagate, TLearning, RSILBudgetBranchingHeuristic3> {
        return CandyBuilder<TPropagate, TLearning, RSILBudgetBranchingHeuristic3>();
    }

    constexpr auto branchWithRSILUnrestricted2() const -> CandyBuilder<TPropagate, TLearning, RSILBranchingHeuristic2> {
        return CandyBuilder<TPropagate, TLearning, RSILBranchingHeuristic2>();
    }

    constexpr auto branchWithRSILBudgeted2() const -> CandyBuilder<TPropagate, TLearning, RSILVanishingBranchingHeuristic2> {
        return CandyBuilder<TPropagate, TLearning, RSILVanishingBranchingHeuristic2>();
    }

    constexpr auto branchWithRSILVanishing2() const -> CandyBuilder<TPropagate, TLearning, RSILBudgetBranchingHeuristic2> {
        return CandyBuilder<TPropagate, TLearning, RSILBudgetBranchingHeuristic2>();
    }

    constexpr auto propagateStaticClauses() const -> CandyBuilder<StaticPropagate, TLearning, TBranching> { 
        return CandyBuilder<StaticPropagate, TLearning, TBranching>();
    }

    CandySolverInterface* build(ClauseDatabase&& database, Trail* assignment) {
        return new Solver<ClauseDatabase, Trail, TPropagate, TLearning, TBranching>(std::move(database), *assignment);
    }

};

CandySolverInterface* createSolver(GlobalClauseAllocator* global_allocator = nullptr, bool staticPropagate = false, bool lrb = false, bool rsil = false);

}

#endif