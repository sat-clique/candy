/*
 * ClauseAllocator.cc
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#include <core/ClauseAllocator.h>

#include <iostream>

namespace Candy {

ClauseAllocator::ClauseAllocator(uint32_t _number_of_pools) :
    pools(_number_of_pools),
    number_of_pools(_number_of_pools),
    stats_active_long(0),
    stats_active_counts(_number_of_pools, 0)
{
    for (uint32_t i = 0; i < number_of_pools; i++) {
        refillPool(i);
    }
}

ClauseAllocator::~ClauseAllocator() {
    for (char* page : pages) {
        delete [] page;
    }
}

void ClauseAllocator::printStatistics() {
    printf("\n========= [Pools usage] =========\n");
    for (size_t i = 0; i < number_of_pools; i++) {
        printf("%u;", stats_active_counts[i]);
    }
    printf("\n========= [Pools maximum] =========\n");
    for (size_t i = 0; i < number_of_pools; i++) {
        printf("%zu;", stats_active_counts[i] + pools[i].size());
    }
    printf("\n========= [Clauses Beyond Pool] =========\n");
    printf("%u\n", stats_active_long);
}

void ClauseAllocator::refillPool(uint16_t index) {
    uint32_t nElem = stats_active_counts[index] + initialNumberOfElements(index);
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

} /* namespace Candy */
