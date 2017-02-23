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

namespace Candy {

class Clause;

class ClauseAllocator {

public:
    ClauseAllocator(uint32_t _number_of_pools, uint32_t _elements_per_pool);
    virtual ~ClauseAllocator();

    void* allocate(uint32_t length);
    void deallocate(Clause*);

    uint32_t clauseBytes(uint32_t length);

private:
    std::vector<std::vector<void*>> pools;
    std::vector<char*> pages;

    uint32_t number_of_pools;
    uint32_t initial_elements_per_pool;

    uint32_t stats_active_total = 0;
    uint32_t stats_active_long = 0;
    std::vector<uint32_t> stats_active_counts;

    std::vector<void*>& getPool(uint16_t index);
    void refillPool(uint16_t index);

    inline uint16_t getPoolIndex(uint32_t size) {
        /*
         * header size is 20
         * literal size is 4
         * index 0: clause of length 0-3 has length 32
         * index 1: clause of length 4-11 has length 64
         * index 2: clause of length 12-19 has length 64 + 32
         * ...
         * minimum space: min = size * 4 + 20
         * aligned space: ali = min + 32 - min % 32
         * index: ali / 32 - 1
         * index: (size + 4) / 8
         */
        return (size + 4) / 8;
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
