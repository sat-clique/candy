/*
 * ClauseAllocator.cc
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#include <core/ClauseAllocator.h>

#include <candy/core/Clause.h>
#include <iostream>

namespace Candy {

ClauseAllocator::ClauseAllocator(uint32_t _number_of_pools, uint32_t _initial_elements_per_pool) :
    number_of_pools(_number_of_pools),
    initial_elements_per_pool(_initial_elements_per_pool),
    stats_active_long(0),
    stats_active_counts(_number_of_pools, 0)
{
    for (uint32_t i = 0; i < number_of_pools; i++) {
        pools.push_back(std::vector<void*>());
        refillPool(initial_elements_per_pool, i+1);
    }
}

ClauseAllocator::~ClauseAllocator() {
    for (char* page : pages) {
        delete [] page;
    }
}

uint32_t ClauseAllocator::clauseBytes(uint32_t length) {
    return (sizeof(Clause) + sizeof(Lit) * (length-1));
}

void* ClauseAllocator::allocate(uint32_t length) {
    if (length-1 < number_of_pools) {
        stats_active_counts[length-1]++;
        std::vector<void*>& pool = getPool(length);
        void* clause = pool.back();
        pool.pop_back();
        return clause;
    }
    else {
        stats_active_long++;
        return new char[clauseBytes(length)];
    }
}

void ClauseAllocator::deallocate(Clause* clause) {
    if (clause->size()-1 < number_of_pools) {
        stats_active_counts[clause->size()-1]--;
        pools[clause->size()-1].push_back(clause);
    }
    else {
        stats_active_long--;
        delete [] clause;
    }
}

std::vector<void*>& ClauseAllocator::getPool(int32_t length) {
    if (pools[length-1].size() == 0) {
        refillPool(stats_active_counts[length-1], length);
    }
    return pools[length-1];
}

void ClauseAllocator::refillPool(int32_t nElem, int32_t length) {
    uint16_t clause_bytes = clauseBytes(length);
    char* pool = new char[clause_bytes * nElem];
    pages.push_back(pool);
    for (char* pos = pool; pos < pool + (clause_bytes * nElem); pos += clause_bytes) {
        pools[length-1].push_back(pos);
    }
}

} /* namespace Candy */
