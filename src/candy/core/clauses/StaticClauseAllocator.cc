#include "candy/core/clauses/ClauseAllocator.h"
#include "candy/core/clauses/StaticClauseAllocator.h"

namespace Candy {
    ClauseAllocator StaticClauseAllocator::allocator { };
}