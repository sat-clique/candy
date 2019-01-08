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

class CandyBuilderInterface {
protected:
    BranchingDiversificationInterface* branching_interface;

public:
    CandySolverInterface* build() = 0;

    /** Access after build: */
    BranchingDiversificationInterface* accessBranchingInterface() {
        assert(branching_interface != nullptr);
        return branching_interface;
    }
}

template<class TClauses = ClauseDatabase<ClauseAllocator>, class TPropagate = Propagate<TClauses>, class TLearning = ConflictAnalysis<TClauses>, class TBranching = VSIDS<TClauses>> 
class CandyBuilder : CandyBuilderInterface { 
public:
    TClauses* database = nullptr;
    Trail* assignment = nullptr;

    TPropagate* propagate = nullptr;
    TLearning* learning = nullptr;
    TBranching* branching = nullptr;

    constexpr CandyBuilder(TClauses* db, Trail* as) : database(db), assignment(as) { }

    constexpr auto branchWithVSIDS() const -> CandyBuilder<TClauses, TPropagate, TLearning, VSIDS<TClauses>> {
        return CandyBuilder<TClauses, TPropagate, TLearning, VSIDS<TClauses>>(database, assignment);
    }

    constexpr auto branchWithLRB() const -> CandyBuilder<TClauses, TPropagate, TLearning, LRB<TClauses>> {
        return CandyBuilder<TClauses, TPropagate, TLearning, LRB<TClauses>>(database, assignment);
    }

    constexpr auto propagateStaticClauses() const -> CandyBuilder<TClauses, StaticPropagate<TClauses>, TLearning, TBranching> { 
        return CandyBuilder<TClauses, StaticPropagate<TClauses>, TLearning, TBranching>(database, assignment);
    }

    CandySolverInterface* build() override {
        TPropagate* propagate = new TPropagate(*database, *assignment);
        TLearning* learning = new TLearning(*database, *assignment);
        TBranching* branching = new TBranching(*database, *assignment);
        branching_interface = branching;
        return new Solver<TClauses, Trail, TPropagate, TLearning, TBranching>(*database, *assignment, *propagate, *learning, *branching);
    }

};

}

#endif