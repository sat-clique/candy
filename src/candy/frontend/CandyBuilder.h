#ifndef CANDY_BUILDER_H_
#define CANDY_BUILDER_H_

#include "candy/simp/SimpSolver.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/core/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/core/Propagate.h"
#include "candy/core/ConflictAnalysis.h"
#include "candy/core/PropagateThreadSafe.h"
#include "candy/core/ConflictAnalysisThreadSafe.h"
#include "candy/core/branching/LRB.h"
#include "candy/core/branching/VSIDS.h"

namespace Candy {

template<class TPropagate = Propagate, class TLearning = ConflictAnalysis, class TBranching = VSIDS>
class CandyBuilder {
public:
    ClauseDatabase* database = nullptr;
    Trail* assignment = nullptr;

    constexpr CandyBuilder(ClauseDatabase* db, Trail* as) : database(db), assignment(as) { }

    constexpr auto branchWithVSIDS() const -> CandyBuilder<TPropagate, TLearning, VSIDS> {
        return CandyBuilder<TPropagate, TLearning, VSIDS>(database, assignment);
    }

    constexpr auto branchWithLRB() const -> CandyBuilder<TPropagate, TLearning, LRB> {
        return CandyBuilder<TPropagate, TLearning, LRB>(database, assignment);
    }

    constexpr auto learnThreadSafe() const -> CandyBuilder<TPropagate, ConflictAnalysisThreadSafe, TBranching> {
        return CandyBuilder<TPropagate, ConflictAnalysisThreadSafe, TBranching>(database, assignment);
    }

    constexpr auto propagateThreadSafe() const -> CandyBuilder<PropagateThreadSafe, TLearning, TBranching> {
        return CandyBuilder<PropagateThreadSafe, TLearning, TBranching>(database, assignment);
    }

    CandySolverInterface* build() {
        return new SimpSolver<ClauseDatabase, Trail, TPropagate, TLearning, TBranching>(*database, *assignment, 
            *new TPropagate(*database, *assignment), *new TLearning(*database, *assignment), *new TBranching(*database, *assignment));
    }

};

}

#endif