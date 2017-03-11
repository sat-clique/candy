/*
 * ClauseAllocator.cc
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#include <core/ClauseAllocator.h>

#include <iostream>

namespace Candy {

ClauseAllocator::ClauseAllocator() :
    pages(2*NUMBER_OF_POOLS),
    pools(NUMBER_OF_POOLS),
    xxl_pool()
{
    for (uint32_t i = 0; i < NUMBER_OF_POOLS; i++) {
        pools[i].reserve(initialNumberOfElements(i));
        fillPool(i);
    }
    xxl_pool.reserve(1024);
    fillXXLPool();
}

ClauseAllocator::~ClauseAllocator() {
    for (char* page : pages) {
        delete [] page;
    }
}

void ClauseAllocator::fillPool(uint16_t index) {
    const uint32_t nElem = pools[index].capacity();
    const uint16_t nLits = 1 + index;
    const uint16_t clause_bytes = sizeof(Clause) - sizeof(Lit) + nLits * sizeof(Lit);
    const uint32_t bytes_total = clause_bytes * nElem;
    char* pool = (char*)malloc(bytes_total);
    pages.push_back(pool);
    for (uint32_t pos = 0; pos < bytes_total; pos += clause_bytes) {
        pools[index].push_back(pool + pos);
    }
}

void ClauseAllocator::fillXXLPool() {
    const uint32_t nElem = xxl_pool.capacity();
    const uint16_t nLits = XXL_POOL_ONE_SIZE;
    const uint16_t clause_bytes = sizeof(Clause) - sizeof(Lit) + nLits * sizeof(Lit);
    const uint32_t bytes_total = clause_bytes * nElem;
    char* pool = (char*)malloc(bytes_total);
    pages.push_back(pool);
    for (uint32_t pos = 0; pos < bytes_total; pos += clause_bytes) {
        xxl_pool.push_back(pool + pos);
    }
}

} /* namespace Candy */
