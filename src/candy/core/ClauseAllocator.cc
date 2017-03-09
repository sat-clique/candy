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
    pools(_number_of_pools),
    number_of_pools(_number_of_pools),
    initial_elements_per_pool(_initial_elements_per_pool),
    stats_active_long(0),
    stats_active_counts(_number_of_pools, 0)
{
    for (uint32_t i = 0; i < number_of_pools; i++) {
        refillPool(i, initial_elements_per_pool);
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
        stats_active_counts[index]--; // stats can be <0 if clauses shrink
        getPool(index).push_back(clause);
    }
    else {
        stats_active_long--;
        free((void*)clause);
    }
}

std::vector<void*>& ClauseAllocator::getPool(uint16_t index) {
    if (pools[index].size() == 0) {
        // workaround clause shrinking (clauses changing pools):
        uint32_t nElem = std::max(stats_active_counts[index], initial_elements_per_pool);
        refillPool(index, nElem);
//        for (int i = 0; i < number_of_pools; i++) {
//            printf("pool %i: use %i of %i; ", i, stats_active_counts[i], stats_active_counts[i] + pools[i].size());
//        }
//        printf("%i exceed pool-handled size\n", stats_active_long);
    }
    return pools[index];
}

void ClauseAllocator::refillPool(uint16_t index, uint32_t nElem) {
    uint16_t nLits = 1 + index;
    //uint16_t nLits = 2 + index * 2;
    uint16_t clause_bytes = sizeof(Clause) - sizeof(Lit) + nLits * sizeof(Lit);
    uint32_t bytes_total = clause_bytes * nElem;
    char* pool = (char*)malloc(bytes_total + clause_bytes); // one more for alignment
    pages.push_back(pool);
    for (uint32_t pos = 0; pos < bytes_total; pos += clause_bytes) {
        pools[index].push_back(pool + pos);
    }
}

/*
 * clause header is 8 bytes
 * literal size is 4 bytes
 * index 0: clause of length 1-2 has length 16
 * index 1: clause of length 3-4 has length 24
 * index 2: clause of length 5-6 has length 32
 * ...
 * index = (length - 1) / 2
 */
uint16_t ClauseAllocator::getPoolIndex(uint32_t size) {
    return size - 1;
    //return (size - 1) / 2;
}

} /* namespace Candy */
