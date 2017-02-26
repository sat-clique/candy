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
    void refillPool(uint16_t index, uint32_t nElem);

    /*
     * clause header is 12 bytes
     * literal size is 4 bytes
     * index 0: clause of length 1 has length 16
     * index 1: clause of length 2-5 has length 32
     * index 2: clause of length 6-9 has length 48
     * index 3: clause of length 10-13 has length 64
     * ...
     * index = (length + 2) / 4
     */
    inline uint16_t getPoolIndex(uint32_t size) {
        return (size + 2) / 4;
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
