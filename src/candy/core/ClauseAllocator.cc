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
    pages3(),
    pages3nelem(),
    pools(),
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
        free(page);
    }
    for (char* page : pages3) {
        free(page);
    }
}

void ClauseAllocator::fillPool(uint16_t index) {
    const uint32_t nElem = pools[index].capacity();
    const uint16_t nLits = 1 + index;
    const uint16_t clause_bytes = clauseBytes(nLits);
    const uint32_t bytes_total = clause_bytes * nElem;
    char* page = (char*)calloc(nElem, clause_bytes);
    if (nLits == 3) {
        pages3.push_back(page);
        pages3nelem.push_back(nElem);
    } else {
        pages.push_back(page);
    }
    for (uint32_t pos = 0; pos < bytes_total; pos += clause_bytes) {
        pools[index].push_back(page + pos);
    }
}

void ClauseAllocator::fillXXLPool() {
    const uint32_t nElem = xxl_pool.capacity();
    const uint16_t clause_bytes = clauseBytes(XXL_POOL_ONE_SIZE);
    const uint32_t bytes_total = clause_bytes * nElem;
    char* page = (char*)malloc(bytes_total);
    pages.push_back(page);
    for (uint32_t pos = 0; pos < bytes_total; pos += clause_bytes) {
        xxl_pool.push_back(page + pos);
    }
}

} /* namespace Candy */
