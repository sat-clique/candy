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

#define NUMBER_OF_POOLS 500
#define XXL_POOL_ONE_SIZE 1000
#define XXL_POOL_INDEX NUMBER_OF_POOLS
#define REVAMPABLE_PAGES_MAX_SIZE 6
#define PAGE_MAX_ELEMENTS 262144

namespace Candy {

class ClauseAllocator {

public:
    ClauseAllocator() :
        pools(),
        pages(),
        pages_nelem()
    {
        for (uint_fast32_t i = 0; i < NUMBER_OF_POOLS; i++) {
            pools[i].reserve(initialNumberOfElements(i));
            fillPool(i);
        }
        pools[XXL_POOL_INDEX].reserve(1024);
        fillPool(XXL_POOL_INDEX);
    }

    ~ClauseAllocator() {
        for (auto pages : this->pages) {
            for (char* page : pages) {
                free(page);
            }
        }
    }

    ClauseAllocator(ClauseAllocator const&) = delete;
    void operator=(ClauseAllocator const&)  = delete;

    inline void* allocate(uint_fast32_t length) {
        if (length > XXL_POOL_ONE_SIZE) {
            Statistics::getInstance().allocatorBeyondMallocdInc();
            return malloc(clauseBytes(length));
        }
        else {
            uint_fast16_t index = getPoolIndex(length);
            std::vector<void*>& pool = pools[index];

            if (index < NUMBER_OF_POOLS) {
                Statistics::getInstance().allocatorPoolAllocdInc(index);
            } else {
                Statistics::getInstance().allocatorXXLPoolAllocdInc();
            }

            if (pool.size() == 0) {
                pool.reserve(std::min(pool.capacity()*2, (size_t)PAGE_MAX_ELEMENTS));
                fillPool(index);
            }

            void* clause = pool.back();
            pool.pop_back();
            return clause;
        }
    }

    inline void deallocate(Clause* clause) {
        if (clause->size() > XXL_POOL_ONE_SIZE) {
            Statistics::getInstance().allocatorBeyondMallocdDec();
            free((void*)clause);
        }
        else {
            uint_fast16_t index = getPoolIndex(clause->size());
            clause->activity() = 0;
            pools[index].push_back(clause);

            if (index < NUMBER_OF_POOLS) {
                Statistics::getInstance().allocatorPoolAllocdDec(index);
            } else {
                Statistics::getInstance().allocatorXXLPoolAllocdDec();
            }
        }
    }

    std::vector<Clause*> revampPages(size_t i);

private:
    std::array<std::vector<void*>, NUMBER_OF_POOLS+1> pools;

    std::array<std::vector<char*>, REVAMPABLE_PAGES_MAX_SIZE+1> pages;
    std::array<std::vector<size_t>, REVAMPABLE_PAGES_MAX_SIZE> pages_nelem;

    void fillPool(uint_fast16_t index);

    inline uint_fast32_t clauseBytes(uint_fast32_t length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }

    inline uint_fast16_t getPoolIndex(uint_fast32_t size) const {
        return std::min(size-1, (uint_fast32_t)XXL_POOL_INDEX);
    }

    inline uint_fast32_t initialNumberOfElements(uint_fast32_t index) {
        if (index < 100) {
            return PAGE_MAX_ELEMENTS >> (index / 10);
        }
        return 256;
    }

    template <unsigned int N> struct SortHelperClause {
        uint16_t length;
        uint16_t header;
        float act;
        Lit literals[N];
    };

    template <unsigned int N> std::vector<Clause*> revampPages();

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
