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
public:
    ClauseAllocator() : old_pages(), pages(), cursor(0), page_size(INITIAL_PAGE_SIZE) {
        newPage();
    }

    ~ClauseAllocator() {
        for (unsigned char* page : pages) {
            std::free((void*)page);
        }
        pages.clear();
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

    void reallocate() {
        if (!old_pages.empty()) return;
        old_pages.swap(pages);
        size_t old_page_size = page_size;
        page_size *= old_pages.size();
        newPage();
        for (unsigned char* old_page : old_pages) {
            unsigned int num_literals = 0;
            unsigned int num_bytes = 0;
            for (unsigned char *p = old_page, *end = old_page + old_page_size; p < end; p += num_bytes) {
                Clause* old_clause = (Clause*)p;
                num_literals = old_clause->size();
                num_bytes = clauseBytes(num_literals);
                if (num_literals == 0) {
                    break;
                }
                else if (!old_clause->isDeleted()) {
                    void* clause = allocate(num_literals);
                    memcpy(clause, (void*)old_clause, num_bytes);
                }
            }
        }
        assert(pages.size() == 1);
    }

    void free_old_pages() {
        for (unsigned char* page : old_pages) {
            std::free((void*)page);
        }
        old_pages.clear();
    }

    inline void enroll() { }

    std::vector<Clause*> collect() {
        std::vector<Clause*> clauses {};
        for (unsigned char* page : pages) {
            unsigned int num_bytes = 0;
            for (unsigned char *p = page, *end = page + page_size; p < end; p += num_bytes) {
                Clause* clause = (Clause*)p;
                if (clause->size() == 0) {
                    break;
                }
                else if (!clause->isDeleted()) {
                    clauses.push_back(clause);
                }
                num_bytes = clauseBytes(clause->size());
            }
        }
        return clauses;
    }

    void reset() {
        for (unsigned char* page : pages) {
            std::free((void*)page);
        }
        pages.clear();
        page_size = INITIAL_PAGE_SIZE;
        newPage();
    }

private:
    static constexpr unsigned int INITIAL_PAGE_SIZE = 64*1024*1024;

    std::vector<unsigned char*> old_pages;
    std::vector<unsigned char*> pages;

    size_t cursor;
    size_t page_size;
    unsigned char* memory;

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

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
