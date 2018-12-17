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
#include <numeric>

#include <candy/core/Clause.h>
#include <candy/core/Statistics.h>

namespace Candy {

class ClauseAllocator {
    friend class ClauseDatabase;

private:
    static constexpr unsigned int INITIAL_PAGE_SIZE = 64*1024*1024;

    std::vector<unsigned char*> pages;

    size_t cursor;
    size_t page_size;
    unsigned char* memory;

    ClauseAllocator() : pages(), cursor(0), page_size(INITIAL_PAGE_SIZE) {
        memory = (unsigned char*)std::malloc(INITIAL_PAGE_SIZE);
        pages.push_back(memory);
    }

    ~ClauseAllocator() {
        for (unsigned char* page : pages) {
            std::free((void*)page);
        }
    }

    ClauseAllocator(ClauseAllocator const&) = delete;
    void operator=(ClauseAllocator const&)  = delete;

    inline void newPage() {
        memory = (unsigned char*)std::malloc(page_size);
        pages.push_back(memory);
        cursor = 0;
    }

    inline unsigned int clauseBytes(unsigned int length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }

    inline void* allocate(unsigned int length) {
        unsigned int size = clauseBytes(length);
        if (cursor + size > page_size) {
            newPage();
        }
        void* clause = memory + cursor;
        cursor += size;
        return clause;
    }

    inline void deallocate(Clause* clause) { 
        // just keep it dangling until defrag does its job
    }

    std::vector<Clause*> defrag(std::vector<Clause*> keep) {
        std::vector<Clause*> keep2 {};
        keep2.reserve(keep.size());
        page_size *= pages.size();
        std::vector<unsigned char*> oldpages;
        oldpages.swap(pages);
        memory = (unsigned char*)std::malloc(page_size);
        pages.push_back(memory);
        cursor = 0;
        for (Clause* clause : keep) {
            unsigned char* pos = memory + cursor;
            unsigned int size = clauseBytes(clause->size());
            memcpy((void*)pos, (void*)clause, size);
            keep2.push_back(reinterpret_cast<Clause*>(pos));
            cursor += size;
        }
        for (unsigned char* page : oldpages) {
            free((void*)page);
        }
        assert(pages.size() == 1);
        return keep2;
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
