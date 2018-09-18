/*
 * ClauseAllocator.h
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CLAUSEALLOCATOR_H_
#define SRC_CANDY_CORE_CLAUSEALLOCATOR_H_

#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <numeric>

#include <candy/core/Clause.h>
#include <candy/core/Statistics.h>

namespace Candy {

class ClauseAllocator {

private:
    static constexpr unsigned int DEFAULT_POOL_SIZE = 70;
    static constexpr unsigned int NUMBER_OF_POOLS = 500;
    static constexpr unsigned int POOL_GROWTH_FACTOR = 2;
    static constexpr unsigned int XXL_POOL_INDEX = 500u;
    static constexpr unsigned int XXL_POOL_ONE_SIZE = 1020;

    std::array<std::vector<void*>, NUMBER_OF_POOLS+1> pools;
    std::vector<char*> pages;

    void fillPool(unsigned int index);
    void preallocateStorage(std::array<unsigned int, 501>& nclauses);

    inline unsigned int clauseBytes(unsigned int length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }

    inline unsigned int getPoolIndex(unsigned int size) const {
        return std::min(size-1, ClauseAllocator::XXL_POOL_INDEX);
    }

public:
    ClauseAllocator();
    ~ClauseAllocator();

    ClauseAllocator(ClauseAllocator const&) = delete;
    void operator=(ClauseAllocator const&)  = delete;

    /**
     * Preallocate additional storage for given list of clauses.
     */
    void announceClauses(const std::vector<std::vector<Lit>*>& problem);
    void announceClauses(const std::vector<Clause*>& clauses);

    inline void* allocate(unsigned int length) {
        if (length <= ClauseAllocator::XXL_POOL_ONE_SIZE) {
            unsigned int index = getPoolIndex(length);
            std::vector<void*>& pool = pools[index];

            Statistics::getInstance().allocatorPoolAlloc(index);

            if (pool.size() == 0) {
                pool.reserve(pool.capacity() * ClauseAllocator::POOL_GROWTH_FACTOR);
                fillPool(index);
            }

            void* clause = pool.back();
            pool.pop_back();
            return clause;
        }
        else {
            Statistics::getInstance().allocatorPoolAlloc(501);
            return malloc(clauseBytes(length));
        }
    }

    inline void deallocate(Clause* clause) {
        if (clause->size() <= ClauseAllocator::XXL_POOL_ONE_SIZE) {
            unsigned int index = getPoolIndex(clause->size());
            clause->activity() = 0;
            pools[index].push_back(clause);
            Statistics::getInstance().allocatorPoolDealloc(index);
        }
        else {
            free((void*)clause);
            Statistics::getInstance().allocatorPoolDealloc(501);
        }
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
