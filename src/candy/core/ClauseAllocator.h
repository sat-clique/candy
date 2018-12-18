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
    void operator=(ClauseAllocator const&) = delete;

    inline void newPage() {
        memory = (unsigned char*)std::malloc(page_size);
        std::memset(memory, 0, page_size);
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

    std::vector<Clause*> defrag() {
        std::vector<Clause*> clauses {};
        size_t old_page_size = page_size;
        std::vector<unsigned char*> old_pages { pages.begin(), pages.end() };
        pages.clear();
        page_size *= old_pages.size();
        newPage();
        for (unsigned char* old_page : old_pages) {
            size_t old_cursor = 0;
            while (old_cursor < old_page_size) {
                void* old_clause = old_page + old_cursor;
                unsigned int num_literals = ((Clause*)old_clause)->size();
                unsigned int num_bytes = clauseBytes(num_literals);
                if (!((Clause*)old_clause)->isDeleted() && num_literals > 0) {
                    void* clause = allocate(num_literals);
                    memcpy(clause, (void*)old_clause, num_bytes);
                    clauses.push_back((Clause*)clause);
                }
                if (num_literals == 0) {
                    old_cursor = old_page_size;
                } else {
                    old_cursor += num_bytes;
                }
            }
        }
        for (unsigned char* page : old_pages) {
            free((void*)page);
        }
        assert(pages.size() == 1);
        return clauses;
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
