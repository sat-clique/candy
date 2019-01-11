#include "candy/frontend/CandyBuilder.h"

namespace Candy {

CandySolverInterface* createSolver(GlobalClauseAllocator* global_allocator, bool staticPropagate, bool lrb) {
    ClauseDatabase* clauses = new ClauseDatabase();
    Trail* assignment = new Trail();

    if (global_allocator != nullptr) {
        clauses->setGlobalClauseAllocator(global_allocator);
    }

    CandyBuilder<> builder { clauses, assignment };

    if (lrb) {
        if (staticPropagate) {
            return builder.branchWithLRB().propagateStaticClauses().build();
        } else {
            return builder.branchWithLRB().build();
        }
    } 
    else {
        if (staticPropagate) {
            return builder.propagateStaticClauses().build();
        } else {
            return builder.build();
        }
    }
}

}