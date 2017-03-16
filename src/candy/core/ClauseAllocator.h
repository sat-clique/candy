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
#define XXL_POOL_INDEX NUMBER_OF_POOLS
#define REVAMPABLE_PAGES_MAX_SIZE 14

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
            if (pools[XXL_POOL_INDEX].size() == 0) {
                pools[XXL_POOL_INDEX].reserve(pools[XXL_POOL_INDEX].capacity()*2);
                fillPool(XXL_POOL_INDEX);
            }
            void* clause = pools[XXL_POOL_INDEX].back();
            pools[XXL_POOL_INDEX].pop_back();
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
//            clause->activity() = 0;
        }
        else if (index < XXL_POOL_ONE_SIZE) {
            Statistics::getInstance().allocatorXXLPoolAllocdDec();
            pools[XXL_POOL_INDEX].push_back(clause);
        }
        else {
            Statistics::getInstance().allocatorBeyondMallocdDec();
            free((void*)clause);
        }
    }

    std::vector<Clause*> revampPages(size_t i) {
        assert(i > 2);
        assert(i <= REVAMPABLE_PAGES_MAX_SIZE);

        switch (i) {
        case 3: return revampPages<3>();
        case 4: return revampPages<4>();
        case 5: return revampPages<5>();
        case 6: return revampPages<6>();
        case 7: return revampPages<7>();
        case 8: return revampPages<8>();
        case 9: return revampPages<9>();
        case 10: return revampPages<10>();
        case 11: return revampPages<11>();
        case 12: return revampPages<12>();
        case 13: return revampPages<13>();
        case 14: return revampPages<14>();
        default: return std::vector<Clause*>();
        }
    }

private:
    ClauseAllocator();

    std::array<std::vector<void*>, NUMBER_OF_POOLS+1> pools;

    std::array<std::vector<char*>, REVAMPABLE_PAGES_MAX_SIZE+1> pages;
    std::array<std::vector<size_t>, REVAMPABLE_PAGES_MAX_SIZE> pages_nelem;

    void fillPool(uint16_t index);

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


    template <unsigned int N> struct SortHelperClause {
        uint16_t length;
        uint16_t header;
        float act;
        Lit literals[N];
    };

    template <unsigned int N> std::vector<Clause*> revampPages() {
        uint16_t index = getPoolIndex(N);
        std::vector<void*>& pool = pools[index];
        std::vector<char*>& page = pages[index];
        std::vector<size_t>& nelem = pages_nelem[index];
        size_t old_pool_size = pool.size();
        pool.clear();
        std::vector<Clause*> revamped;
        for (size_t i = 0; i < page.size(); i++) {
            SortHelperClause<N>* begin = reinterpret_cast<SortHelperClause<N>*>(page[i]);
            SortHelperClause<N>* end = reinterpret_cast<SortHelperClause<N>*>(page[i])  + nelem[i];
            std::sort(begin, end, [](SortHelperClause<N> c1, SortHelperClause<N> c2) { return c1.act > c2.act; });
            for (SortHelperClause<N>* iter = begin; iter < end; iter++) {
                Clause* clause = (Clause*)iter;
                assert(clause->size() == 0 || clause->size() == N);
                if (!clause->isDeleted() && clause->size() > 0) {
                    revamped.push_back(clause);
                } else {
                    pool.push_back(clause);
                }
            }
        }
        assert(old_pool_size == pool.size());
        (void)(old_pool_size);
        return revamped;
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
