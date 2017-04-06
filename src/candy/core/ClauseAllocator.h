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

#include <candy/core/Clause.h>
#include <candy/core/Statistics.h>

#define NUMBER_OF_POOLS 500
#define XXL_POOL_ONE_SIZE 1000
#define XXL_POOL_INDEX NUMBER_OF_POOLS
#define REVAMPABLE_PAGES_MAX_SIZE 6
#define PAGE_MAX_ELEMENTS 524288
#define REALLOC_ON_REVAMP

namespace Candy {

class ClauseAllocator {

public:
    ClauseAllocator() :
        pools(),
        pages(),
        pages_nelem()
    {
        for (uint32_t i = 0; i < NUMBER_OF_POOLS; i++) {
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

    inline void* allocate(uint32_t length) {
        if (length > XXL_POOL_ONE_SIZE) {
            Statistics::getInstance().allocatorBeyondMallocdInc();
            return malloc(clauseBytes(length));
        }
        else {
            uint16_t index = getPoolIndex(length);
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
            uint16_t index = getPoolIndex(clause->size());
            clause->activity() = 0;
            pools[index].push_back(clause);

            if (index < NUMBER_OF_POOLS) {
                Statistics::getInstance().allocatorPoolAllocdDec(index);
            } else {
                Statistics::getInstance().allocatorXXLPoolAllocdDec();
            }
        }
    }

    std::vector<Clause*> revampPages(size_t i) {
        assert(i > 2);
        assert(i <= REVAMPABLE_PAGES_MAX_SIZE);

        switch (i) {
#ifdef REALLOC_ON_REVAMP
        case 3: return revampPages2<3>();
        case 4: return revampPages2<4>();
        case 5: return revampPages2<5>();
        case 6: return revampPages2<6>();
        case 7: return revampPages2<7>();
        case 8: return revampPages2<8>();
        case 9: return revampPages2<9>();
        case 10: return revampPages2<10>();
        case 11: return revampPages2<11>();
        case 12: return revampPages2<12>();
        case 13: return revampPages2<13>();
        case 14: return revampPages2<14>();
#else
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
#endif
        default: return std::vector<Clause*>();
        }
    }

private:
    std::array<std::vector<void*>, NUMBER_OF_POOLS+1> pools;

    std::array<std::vector<char*>, REVAMPABLE_PAGES_MAX_SIZE+1> pages;
    std::array<std::vector<size_t>, REVAMPABLE_PAGES_MAX_SIZE> pages_nelem;

    void fillPool(uint16_t index);

    inline uint32_t clauseBytes(uint32_t length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }

    inline uint16_t getPoolIndex(uint32_t size) const {
        return std::min(size-1, (uint32_t)XXL_POOL_INDEX);
    }

    uint32_t initialNumberOfElements(uint32_t index) {
        if (index > 2 && index < 120) {
            return 262144 >> (index / 10);
        } else if (index == 1 || index == 2) {
            return PAGE_MAX_ELEMENTS;
        }
        return 256;
    }


    template <unsigned int N> struct SortHelperClause {
        uint16_t length;
        uint16_t header;
        float act;
        Lit literals[N];
    };

    template <unsigned int N> inline std::vector<Clause*> revampPages() {
#if REVAMPABLE_PAGES_MAX_SIZE >= N
        uint16_t index = getPoolIndex(N);
        std::vector<void*>& pool = pools[index];
        std::vector<char*>& page = pages[index];
        std::vector<size_t>& nelem = pages_nelem[index];
        size_t old_pool_size = pool.size();
        pool.clear();
        for (size_t i = 0; i < page.size(); i++) {
            SortHelperClause<N>* begin = reinterpret_cast<SortHelperClause<N>*>(page[i]);
            SortHelperClause<N>* end = reinterpret_cast<SortHelperClause<N>*>(page[i])  + nelem[i];
            std::sort(begin, end, [](SortHelperClause<N> c1, SortHelperClause<N> c2) { return c1.act > c2.act; });
        }
        std::vector<Clause*> revamped;
        for (size_t i = 0; i < page.size(); i++) {
            SortHelperClause<N>* begin = reinterpret_cast<SortHelperClause<N>*>(page[i]);
            SortHelperClause<N>* end = reinterpret_cast<SortHelperClause<N>*>(page[i])  + nelem[i];
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
#else
        return std::vector<Clause*>();
#endif
    }

    template <unsigned int N> inline std::vector<Clause*> revampPages2() {
#if REVAMPABLE_PAGES_MAX_SIZE >= N
        uint16_t index = getPoolIndex(N);
        std::vector<void*>& pool = pools[index];
        std::vector<char*>& page = pages[index];
        std::vector<size_t>& nelem = pages_nelem[index];
        size_t old_pool_size = pool.size();
        pool.clear();

        char* new_page = page[0];
        size_t total_nelems = nelem[0];
        if (page.size() > 1) {
            total_nelems = std::accumulate(nelem.begin(), nelem.end(), (size_t)0);
            new_page = (char*)calloc(total_nelems, sizeof(SortHelperClause<N>));
            SortHelperClause<N>* pos = reinterpret_cast<SortHelperClause<N>*>(new_page);
            for (size_t i = 0; i < page.size(); i++) {
                SortHelperClause<N>* begin = reinterpret_cast<SortHelperClause<N>*>(page[i]);
                memcpy(pos, begin, nelem[i] * sizeof(SortHelperClause<N>));
                pos += nelem[i];
            }

            for (size_t i = 0; i < page.size(); i++) {
                delete page[i];
            }
            page.clear();
            nelem.clear();
            page.push_back(new_page);
            nelem.push_back(total_nelems);
        }

        SortHelperClause<N>* begin = reinterpret_cast<SortHelperClause<N>*>(new_page);
        SortHelperClause<N>* end = reinterpret_cast<SortHelperClause<N>*>(new_page)  + total_nelems;
        std::sort(begin, end, [](SortHelperClause<N> c1, SortHelperClause<N> c2) { return c1.act > c2.act; });

        std::vector<Clause*> revamped;
        for (SortHelperClause<N>* iter = begin; iter < end; iter++) {
            Clause* clause = (Clause*)iter;
            assert(clause->size() == 0 || clause->size() == N);
            if (!clause->isDeleted() && clause->size() > 0) {
                revamped.push_back(clause);
            } else {
                pool.push_back(clause);
            }
        }

        assert(old_pool_size == pool.size());
        (void)(old_pool_size);
        return revamped;
#else
        return std::vector<Clause*>();
#endif
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
