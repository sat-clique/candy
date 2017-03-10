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

    void printStatistics();

    inline void* allocate(uint32_t length) {
        uint16_t index = getPoolIndex(length);
        if (index < NUMBER_OF_POOLS) {
            stats_active_counts[index]++;
            std::vector<void*>& pool = pools[index];

            if (pool.size() == 0) {
                fillPool(index);
            }

            void* clause = pool.back();
            pool.pop_back();
            return clause;
        }
        else if (index < XXL_POOL_ONE_SIZE) {
            stats_active_xxl++;
            if (xxl_pool.size() == 0) {
                fillXXLPool();
            }
            void* clause = xxl_pool.back();
            xxl_pool.pop_back();
            return clause;
        }
        else {
            stats_active_beyond++;
            return malloc(clauseBytes(length));
        }
    }

    inline void deallocate(Clause* clause) {
        uint16_t index = getPoolIndex(clause->size());
        if (index < NUMBER_OF_POOLS) {
            stats_active_counts[index]--; // stats can be <0 if clause was shrinked
            pools[index].push_back(clause);
        }
        else if (index < XXL_POOL_ONE_SIZE) {
            stats_active_xxl--;
            xxl_pool.push_back(clause);
        }
        else {
            stats_active_beyond--;
            free((void*)clause);
        }
    }

    // keep statistics intact (although clause now leaks one literal)
    inline void strengthen(uint32_t length) {
        uint16_t index = getPoolIndex(length);
        if (index < NUMBER_OF_POOLS) {
            stats_active_counts[index]--;
            stats_active_counts[index-1]++;
        }
        else if (index < XXL_POOL_ONE_SIZE && index-1 < NUMBER_OF_POOLS) {
            stats_active_xxl--;
            stats_active_counts[index-1]++;
        }
        else if (index-1 < XXL_POOL_ONE_SIZE) {
            stats_active_beyond--;
            stats_active_xxl++;
        }
    }

private:
    ClauseAllocator();

    std::vector<char*> pages;

    std::vector<std::vector<void*>> pools;
    std::vector<void*> xxl_pool;

    std::vector<uint32_t> stats_active_counts;
    uint32_t stats_active_xxl;
    uint32_t stats_active_beyond;

    void fillPool(uint16_t index);
    void fillXXLPool();

    inline uint32_t clauseBytes(uint32_t length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }

    inline uint16_t getPoolIndex(uint32_t size) {
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
