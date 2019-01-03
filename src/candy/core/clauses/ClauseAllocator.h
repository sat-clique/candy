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

#include <candy/core/clauses/Clause.h>
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
        std::memset(memory, 0, INITIAL_PAGE_SIZE);
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
        std::vector<unsigned char*> old_pages { };
        old_pages.swap(pages);
        size_t old_page_size = page_size;
        page_size *= old_pages.size();
        newPage();
        for (unsigned char* old_page : old_pages) {
            unsigned int num_literals = 0;
            unsigned int num_bytes = 0;
            for (size_t old_cursor = 0; old_cursor < old_page_size; old_cursor += num_bytes) {
                Clause* old_clause = (Clause*)(old_page + old_cursor);
                num_literals = old_clause->size();
                num_bytes = clauseBytes(num_literals);
                if (num_literals == 0) {
                    break;
                }
                else if (!old_clause->isDeleted()) {
                    void* clause = allocate(num_literals);
                    memcpy(clause, (void*)old_clause, num_bytes);
                    clauses.push_back((Clause*)clause);
                }
            }
        }
        for (unsigned char* page : old_pages) {
            std::free((void*)page);
        }
        assert(pages.size() == 1);
        return clauses;
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
