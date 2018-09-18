/*
 * ClauseAllocator.cc
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#include <candy/core/ClauseAllocator.h>

#include <iostream>
#include <algorithm>

namespace Candy {

constexpr unsigned int ClauseAllocator::DEFAULT_POOL_SIZE;
constexpr unsigned int ClauseAllocator::NUMBER_OF_POOLS;
constexpr unsigned int ClauseAllocator::POOL_GROWTH_FACTOR;
constexpr unsigned int ClauseAllocator::XXL_POOL_INDEX;
constexpr unsigned int ClauseAllocator::XXL_POOL_ONE_SIZE;

ClauseAllocator::ClauseAllocator() : pools(), pages() {
    for (auto& pool : pools) {
        pool.reserve(ClauseAllocator::DEFAULT_POOL_SIZE);
    }
}

ClauseAllocator::~ClauseAllocator() {
    for (char* page : pages) {
        free(page);
    }
}

void ClauseAllocator::preallocateStorage(std::array<unsigned int, 501>& nclauses) {
    for (unsigned int i = 1; i < pools.size(); i++) {
        pools[i].reserve(std::max(ClauseAllocator::DEFAULT_POOL_SIZE, nclauses[i] * ClauseAllocator::POOL_GROWTH_FACTOR));
        fillPool(i);
    }
}

void ClauseAllocator::announceClauses(const std::vector<std::vector<Lit>*>& problem) {
    std::array<unsigned int, 501> nclauses;
    nclauses.fill(0);
    for (std::vector<Lit>* clause : problem) {
        nclauses[std::min(clause->size(), (size_t)501)-1]++;
    }
    this->preallocateStorage(nclauses);
}

void ClauseAllocator::announceClauses(const std::vector<Clause*>& clauses) {
    std::array<unsigned int, 501> nclauses;
    nclauses.fill(0);
    for (Clause* clause : clauses) {
        nclauses[std::min(clause->size(), (uint16_t)501)-1]++;
    }
    this->preallocateStorage(nclauses);
}

void ClauseAllocator::fillPool(unsigned int index) {
    const unsigned int nElem = pools[index].capacity() - pools[index].size();
    const unsigned int nLits = index != XXL_POOL_INDEX ? 1 + index : ClauseAllocator::XXL_POOL_ONE_SIZE;
    const unsigned int clause_bytes = clauseBytes(nLits);
    const unsigned int bytes_total = clause_bytes * nElem;
    char* page = (char*)calloc(nElem, clause_bytes);
    pages.push_back(page);
    for (unsigned int pos = 0; pos < bytes_total; pos += clause_bytes) {
        pools[index].push_back(page + pos);
    }
}

} /* namespace Candy */
