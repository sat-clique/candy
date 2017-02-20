/*
 * ClauseAllocator.h
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CLAUSEALLOCATOR_H_
#define SRC_CANDY_CORE_CLAUSEALLOCATOR_H_

#include <vector>

namespace Candy {

class Clause;

class ClauseAllocator {

    std::vector<std::vector<Clause*>> pools;
    std::vector<void*> pages;

public:
    ClauseAllocator(uint16_t max_sized_pools, uint16_t elements_per_pool=1000000);
    virtual ~ClauseAllocator();

    Clause* allocate(int32_t length);
    void deallocate(Clause*);

private:
    uint16_t number_of_pools;
    uint16_t elements_per_pool;

    void refillPool(int32_t clause_size);

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
