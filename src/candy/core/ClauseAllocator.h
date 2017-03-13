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
    std::vector<Clause*> revampPages3() {
        struct CastHelperClause {
            uint16_t length;
            uint16_t header;
            float act;
            Lit literals[3];
        };
        assert(sizeof(CastHelperClause)-2*sizeof(Lit) == sizeof(Clause));
        for (size_t i = 0; i < pages3.size(); i++) {
            CastHelperClause* casted = reinterpret_cast<CastHelperClause*>(pages3[i]);
            assert(casted->length == 3 || casted->length == 0);
            std::sort(casted, casted + pages3nelem[i], [](CastHelperClause c1, CastHelperClause c2) { assert(c1.length == 3 || c1.length == 0); assert(c2.length == 3 || c2.length == 0); return c1.act > c2.act; });
        }
        std::vector<Clause*> revamped;
        for (size_t i = 0; i < pages3.size(); i++) {
            char* page = pages3[i];
            const uint32_t bytes_total = clauseBytes(3) * pages3nelem[i];
            for (uint32_t pos = 0; pos < bytes_total; pos += clauseBytes(3)) {
                Clause* clause = (Clause*)(page + pos);
                assert(clause->size() == 0 || clause->size() == 3);
                if (!clause->isDeleted() && clause->size() > 0) {
                    revamped.push_back(clause);
                }
            }
        }
        return revamped;
    }

private:
    ClauseAllocator();

    std::vector<char*> pages;

    std::vector<char*> pages3;
    std::vector<uint32_t> pages3nelem;

    std::array<std::vector<void*>, NUMBER_OF_POOLS> pools;
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
