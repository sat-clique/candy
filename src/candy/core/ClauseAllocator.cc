/*
 * ClauseAllocator.cc
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#include <candy/core/ClauseAllocator.h>

#include <iostream>

namespace Candy {

void ClauseAllocator::fillPool(uint16_t index) {
    const uint32_t nElem = pools[index].capacity();
    uint16_t nLits = 1 + index;
    if (index == XXL_POOL_INDEX) nLits = XXL_POOL_ONE_SIZE;
    const uint16_t clause_bytes = clauseBytes(nLits);
    const uint32_t bytes_total = clause_bytes * nElem;
    char* page = (char*)calloc(nElem, clause_bytes);
    if (index < REVAMPABLE_PAGES_MAX_SIZE) {
        pages[index].push_back(page);
        pages_nelem[index].push_back(nElem);
    } else {
        pages[REVAMPABLE_PAGES_MAX_SIZE].push_back(page);
    }
    for (uint32_t pos = 0; pos < bytes_total; pos += clause_bytes) {
        pools[index].push_back(page + pos);
    }
}

} /* namespace Candy */
