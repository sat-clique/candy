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
        refillPool(i);
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
    uint16_t index = getPoolIndex(length);
    if (index < number_of_pools) {
        stats_active_counts[index]++;
        std::vector<void*>& pool = getPool(index);
        void* clause = pool.back();
        pool.pop_back();
        return clause;
    }
    else {
        stats_active_long++;
        return malloc(clauseBytes(length));
    }
}

void ClauseAllocator::deallocate(Clause* clause) {
    uint16_t index = getPoolIndex(clause->size());
    if (index < number_of_pools) {
        stats_active_counts[index]--;
        getPool(index).push_back(clause);
    }
    else {
        stats_active_long--;
        free((void*)clause);
    }
}

std::vector<void*>& ClauseAllocator::getPool(uint16_t index) {
    if (pools[index].size() == 0) {
        refillPool(index);
    }
    return pools[index];
}

/*
 * header size is 20
 * literal size is 4
 * index 0: clause of length 0-3 has length 32
 * index 1: clause of length 4-11 has length 64
 * index 2: clause of length 12-19 has length 64 + 32
 * ...
 * minimum space: min = size * 4 + 20
 * aligned space: ali = min + 32 - min % 32
 * index: ali / 32 - 1
 * index: (size + 4) / 8
 */
void ClauseAllocator::refillPool(uint16_t index) {
    uint32_t nElem = stats_active_counts[index];
    uint16_t clause_bytes = (index+1)*32;
    uint32_t bytes_total = clause_bytes * nElem;
    char* pool = (char*)malloc(bytes_total + clause_bytes);
    pages.push_back(pool);
    char* align = (char*)((((uintptr_t)pool) + 32) & ~(static_cast<uintptr_t>(31)));
    assert((uintptr_t)align - (uintptr_t)pool > 0);
    assert((uintptr_t)align - (uintptr_t)pool <= 32);
    for (uint32_t pos = 0; pos < bytes_total; pos += clause_bytes) {
        pools[index].push_back(align + pos);
    }
}

} /* namespace Candy */
