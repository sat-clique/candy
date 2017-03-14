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
    pages(),
    pages_nelem(),
    pools()
{
    for (uint32_t i = 0; i < NUMBER_OF_POOLS; i++) {
        pools[i].reserve(initialNumberOfElements(i));
        fillPool(i);
    }
    pools[XXL_POOL_INDEX].reserve(1024);
    fillPool(XXL_POOL_INDEX);
}

ClauseAllocator::~ClauseAllocator() {
    for (auto pages : this->pages) {
        for (char* page : pages) {
            free(page);
        }
    }
}

void ClauseAllocator::fillPool(uint16_t index) {
    const uint32_t nElem = pools[index].capacity();
    uint16_t nLits = 1 + index;
    if (index == XXL_POOL_INDEX) nLits = XXL_POOL_ONE_SIZE;
    const uint16_t clause_bytes = clauseBytes(nLits);
    const uint32_t bytes_total = clause_bytes * nElem;
    char* page = (char*)calloc(nElem, clause_bytes);
    pages[index].push_back(page);
    pages_nelem[index].push_back(nElem);
    for (uint32_t pos = 0; pos < bytes_total; pos += clause_bytes) {
        pools[index].push_back(page + pos);
    }
}

} /* namespace Candy */
