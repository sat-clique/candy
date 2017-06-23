/*
 * ClauseAllocator.cc
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#include <candy/core/ClauseAllocator.h>

#include <iostream>

namespace Candy {

void ClauseAllocator::fillPool(uint_fast16_t index) {
    const uint_fast32_t nElem = pools[index].capacity();
    uint_fast16_t nLits = 1 + index;
    if (index == XXL_POOL_INDEX) nLits = XXL_POOL_ONE_SIZE;
    const uint16_t clause_bytes = clauseBytes(nLits);
    const uint_fast32_t bytes_total = clause_bytes * nElem;
    char* page = (char*)calloc(nElem, clause_bytes);
    if (index < REVAMPABLE_PAGES_MAX_SIZE) {
        pages[index].push_back(page);
        pages_nelem[index].push_back(nElem);
    } else {
        pages[REVAMPABLE_PAGES_MAX_SIZE].push_back(page);
    }
    for (uint_fast32_t pos = 0; pos < bytes_total; pos += clause_bytes) {
        pools[index].push_back(page + pos);
    }
}

template <unsigned int N> std::vector<Clause*> ClauseAllocator::revampPages() {
    uint_fast16_t index = getPoolIndex(N);
    std::vector<void*>& pool = pools[index];
    std::vector<char*>& page = pages[index];
    std::vector<size_t>& nelem = pages_nelem[index];
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
    SortHelperClause<N>* end = reinterpret_cast<SortHelperClause<N>*>(new_page) + total_nelems;
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

    return revamped;
}

std::vector<Clause*> ClauseAllocator::revampPages(size_t i) {
    assert(i > 2);
    assert(i <= REVAMPABLE_PAGES_MAX_SIZE);

    switch (i) {
    case 3: return revampPages<3>();
    case 4: return revampPages<4>();
    case 5: return revampPages<5>();
    case 6: return revampPages<6>();
    default: return std::vector<Clause*>();
    }
}

} /* namespace Candy */
