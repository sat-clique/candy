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

namespace Candy {

class ClauseAllocator {

public:
    inline static ClauseAllocator& getInstance() {
        static ClauseAllocator allocator(600, 2000);
        return allocator;
    }


    ClauseAllocator(ClauseAllocator const&) = delete;
    void operator=(ClauseAllocator const&)  = delete;

    ~ClauseAllocator();

    void printStatistics();

    inline void* allocate(uint32_t length) {
        uint16_t index = getPoolIndex(length);
        if (index < number_of_pools) {
            stats_active_counts[index]++;
            std::vector<void*>& pool = pools[index];

            if (pool.size() == 0) {
                refillPool(index);
            }

            void* clause = pool.back();
            pool.pop_back();
            return clause;
        }
        else {
            stats_active_long++;
            return malloc(clauseBytes(length));
        }
    }

    inline void deallocate(Clause* clause) {
        uint16_t index = getPoolIndex(clause->size());
        if (index < number_of_pools) {
            stats_active_counts[index]--; // stats can be <0 if clause was shrinked
            pools[index].push_back(clause);
        }
        else {
            stats_active_long--;
            free((void*)clause);
        }
    }

    // keep statistics intact (although clause now leaks one literal)
    inline void strengthen(uint32_t length) {
        uint16_t index = getPoolIndex(length);
        stats_active_counts[index]--;
        stats_active_counts[index-1]++;
    }

private:
    ClauseAllocator(uint32_t _number_of_pools, uint32_t _elements_per_pool);

    std::vector<std::vector<void*>> pools;
    std::vector<char*> pages;

    uint32_t number_of_pools;
    uint32_t initial_elements_per_pool;

    uint32_t stats_active_long = 0;
    std::vector<uint32_t> stats_active_counts;

    void refillPool(uint16_t index);

    inline uint32_t clauseBytes(uint32_t length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }


    /*
     * clause header is 8 bytes
     * literal size is 4 bytes
     * index 0: clause of length 1-2 has length 16
     * index 1: clause of length 3-4 has length 24
     * index 2: clause of length 5-6 has length 32
     * ...
     * index = (length - 1) / 2
     */
    inline uint16_t getPoolIndex(uint32_t size) {
        return size - 1;// go back to pool per size
        //return (size - 1) / 2;
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
