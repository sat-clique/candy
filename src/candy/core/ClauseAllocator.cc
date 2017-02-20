/*
 * ClauseAllocator.cc
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#include <core/ClauseAllocator.h>

#include <Clause.h>

namespace Candy {

ClauseAllocator::ClauseAllocator(uint16_t number_of_pools, uint16_t elements_per_pool) {
    this->number_of_pools = number_of_pools;
    this->elements_per_pool = elements_per_pool;
    std::vector<std::vector<Clause*>> pools(number_of_pools);
    for (int i = 0; i < number_of_pools; i++) {
        refillPool(i+1);
    }
}

ClauseAllocator::~ClauseAllocator() {
    for (void* page : pages) {
        free(page);
    }
}

Clause* ClauseAllocator::allocate(int32_t length) {
    if (length-1 < number_of_pools) {
        if (pools[length-1].size() == 0) refillPool(length);
        Clause* clause = pools[length-1].back();
        pools[length-1].pop_back();
        return clause;
    } else {
        return malloc(sizeof(Clause) + sizeof(Lit) * (length-1));
    }
}

void ClauseAllocator::deallocate(Clause* clause) {
    if (clause->length-1 < number_of_pools) {
        pools[clause->length - 1].push_back(clause);
    }
    else {
        free(clause);
    }
}

void ClauseAllocator::refillPool(int32_t length) {
    uint16_t clause_bytes = (sizeof(Clause) + sizeof(Lit) * (length-1));
    void* pool = malloc(clause_bytes * elements_per_pool);
    pages.push_back(pool);
    for (void* pos = pool; pos < pool + (clause_bytes * elements_per_pool); pos += clause_bytes) {
        pools[length-1].push_back((Clause*)pos);
    }
}

} /* namespace Candy */
