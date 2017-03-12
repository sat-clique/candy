/*
 * ClauseAllocator.h
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CLAUSEALLOCATOR_H_
#define SRC_CANDY_CORE_CLAUSEALLOCATOR_H_

#include <vector>
#include <cstdint>

#include <candy/core/Clause.h>
#include <candy/core/Statistics.h>

#define NUMBER_OF_POOLS 500
#define XXL_POOL_ONE_SIZE 1000

namespace Candy {

class ClauseAllocator {

public:
    inline static ClauseAllocator& getInstance() {
        static ClauseAllocator allocator;
        return allocator;
    }


    ClauseAllocator(ClauseAllocator const&) = delete;
    void operator=(ClauseAllocator const&)  = delete;

    ~ClauseAllocator();

    inline void* allocate(uint32_t length) {
        uint16_t index = getPoolIndex(length);
        if (index < NUMBER_OF_POOLS) {
            Statistics::getInstance().allocatorPoolAllocdInc(index);
            std::vector<void*>& pool = pools[index];

            if (pool.size() == 0) {
                pool.reserve(pool.capacity()*2);
                fillPool(index);
            }

            void* clause = pool.back();
            pool.pop_back();
            return clause;
        }
        else if (index < XXL_POOL_ONE_SIZE) {
            Statistics::getInstance().allocatorXXLPoolAllocdInc();
            if (xxl_pool.size() == 0) {
                xxl_pool.reserve(xxl_pool.capacity()*2);
                fillXXLPool();
            }
            void* clause = xxl_pool.back();
            xxl_pool.pop_back();
            return clause;
        }
        else {
            Statistics::getInstance().allocatorBeyondMallocdInc();
            return malloc(clauseBytes(length));
        }
    }

    inline void deallocate(Clause* clause) {
        uint16_t index = getPoolIndex(clause->size());
        if (index < NUMBER_OF_POOLS) {
            Statistics::getInstance().allocatorPoolAllocdDec(index);
            pools[index].push_back(clause);
        }
        else if (index < XXL_POOL_ONE_SIZE) {
            Statistics::getInstance().allocatorXXLPoolAllocdDec();
            xxl_pool.push_back(clause);
        }
        else {
            Statistics::getInstance().allocatorBeyondMallocdDec();
            free((void*)clause);
        }
    }

private:
    ClauseAllocator();

    std::vector<char*> pages;

    std::vector<std::vector<void*>> pools;
    std::vector<void*> xxl_pool;

    void fillPool(uint16_t index);
    void fillXXLPool();

    inline uint32_t clauseBytes(uint32_t length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }

    inline uint16_t getPoolIndex(uint32_t size) const {
        return size - 1;
    }

    uint32_t initialNumberOfElements(uint32_t index) {
        if (index > 2 && index < 120) {
            return 262144 >> (index / 10);
        } else if (index == 1 || index == 2) {
            return 524288;
        }
        return 256;
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
