#ifndef CANDY_BUILDER_H_
#define CANDY_BUILDER_H_

#include "candy/simp/SimpSolver.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/core/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/core/Propagate.h"
#include "candy/core/ConflictAnalysis.h"
#include "candy/core/branching/LRB.h"
#include "candy/core/branching/VSIDS.h"

namespace Candy {

template<class TClauseDatabase = ClauseDatabase, class TAssignment = Trail, class TPropagate = Propagate, class TLearning = ConflictAnalysis, class TBranching = VSIDS>
class CandyBuilder {
public:
    TClauseDatabase* database = nullptr;
    TAssignment* assignment = nullptr;
    TPropagate* propagate = nullptr;
    TLearning* learning = nullptr;
    TBranching* branching = nullptr;

    constexpr CandyBuilder(TClauseDatabase* db, TAssignment* as, TPropagate* pr, TLearning* le, TBranching* br) 
        : database(db), assignment(as), propagate(pr), learning(le), branching(br)
    {}

    constexpr auto branchWithVSIDS(VSIDS* _branching) const -> CandyBuilder<TClauseDatabase, TAssignment, TPropagate, TLearning, VSIDS> {
        return CandyBuilder(database, assignment, propagate, learning, _branching);
    }

    constexpr auto branchWithLRB(LRB* _branching) const -> CandyBuilder<TClauseDatabase, TAssignment, TPropagate, TLearning, LRB> {
        return CandyBuilder(database, assignment, propagate, learning, _branching);
    }

    constexpr Solver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>* build() const {
        TClauseDatabase* _database = nullptr == database ? new TClauseDatabase() : database;
        TAssignment* _assignment = nullptr == assignment ? new TAssignment() : assignment;
        TPropagate* _propagate = nullptr == propagate ? new TPropagate() : propagate;
        TLearning* _learning = nullptr == learning ? new TLearning() : learning;
        TBranching* _branching = nullptr == branching ? new TBranching() : branching;
        return new SimpSolver<TClauseDatabase, TAssignment, TPropagate, TLearning, TBranching>(*_database, *_assignment, *_propagate, *_learning, *_branching);
    }

};

}

#endif