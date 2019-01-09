#include "candy/core/clauses/ClauseAllocator.h"
#include "candy/core/clauses/StaticClauseAllocator.h"

namespace Candy {
    ClauseAllocator StaticClauseAllocator::allocator { };
    std::mutex StaticClauseAllocator::alock { };
    std::unordered_map<std::thread::id, bool> StaticClauseAllocator::users { };
}