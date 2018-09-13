#include <vector>

#include "candy/core/Clause.h"
#include "candy/core/ClauseAllocator.h"

namespace Candy {

class ClauseDatabase {

public:

    ClauseAllocator allocator;
 
    std::vector<Clause*> clauses; // List of problem clauses.
    std::vector<Clause*> learnts; // List of learnt clauses.
    std::vector<Clause*> persist; // List of binary learnt clauses.

    ClauseDatabase() : allocator(), clauses(), learnts(), persist() {

    }

    ~ClauseDatabase() {
        for (Clause* c : clauses) {
            allocator.deallocate(c);
        }
        for (Clause* c : learnts) {
            allocator.deallocate(c);
        }
        for (Clause* c : persist) {
            allocator.deallocate(c);
        }
    }

};

}