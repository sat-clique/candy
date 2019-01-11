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

namespace Candy {

template<class TClauses = ClauseDatabase, class TPropagate = Propagate, class TLearning = ConflictAnalysis, class TBranching = VSIDS> 
class CandyBuilder { 
public:
    TClauses* database = nullptr;
    Trail* assignment = nullptr;

    constexpr CandyBuilder(TClauses* db, Trail* as) : database(db), assignment(as) { }

    constexpr auto branchWithVSIDS() const -> CandyBuilder<TClauses, TPropagate, TLearning, VSIDS> {
        return CandyBuilder<TClauses, TPropagate, TLearning, VSIDS>(database, assignment);
    }

    constexpr auto branchWithLRB() const -> CandyBuilder<TClauses, TPropagate, TLearning, LRB> {
        return CandyBuilder<TClauses, TPropagate, TLearning, LRB>(database, assignment);
    }

    constexpr auto propagateStaticClauses() const -> CandyBuilder<TClauses, StaticPropagate, TLearning, TBranching> { 
        return CandyBuilder<TClauses, StaticPropagate, TLearning, TBranching>(database, assignment);
    }

    CandySolverInterface* build() {
        TPropagate* propagate = new TPropagate(*database, *assignment);
        TLearning* learning = new TLearning(*database, *assignment);
        TBranching* branching = new TBranching(*database, *assignment);
        return new Solver<TClauses, Trail, TPropagate, TLearning, TBranching>(*database, *assignment, *propagate, *learning, *branching);
    }

};

CandySolverInterface* createSolver(GlobalClauseAllocator* global_allocator, bool staticPropagate, bool lrb);

}

#endif