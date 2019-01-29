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

class Page {
    friend class ClauseAllocator;

private:
    size_t page_size;
    size_t cursor;
    unsigned char* memory;

    Page(Page const&) = delete;
    void operator=(Page const&) = delete;

public:
    Page(size_t page_size_) : page_size(page_size_), cursor(0) {
        memory = (unsigned char*)std::malloc(page_size);
        std::memset(memory, 0, page_size);
    }

    Page(Page&& other) : page_size(other.page_size), cursor(other.cursor), memory(other.memory) {
        other.page_size = 0;
        other.cursor = 0;
        other.memory = nullptr;
    }

    ~Page() {
        if (memory != nullptr) {
            std::free((void*)memory);
            memory = nullptr;
        }
    }

    bool hasMemory(size_t bytes) {
        assert(memory != nullptr);
        return cursor + bytes < page_size;
    }

    void* getMemory(size_t bytes) {
        assert(memory != nullptr);
        void* result = memory + cursor;
        cursor += bytes;
        return result;
    }

    bool contains(void* p) {
        assert(memory != nullptr);
        return p >= (void*)memory && p < (void*)(memory + cursor);
    }
};

class ClauseAllocator {
public:
    ClauseAllocator() : pages(), deleted() {
        pages.emplace_back(PAGE_SIZE);
    }

    ~ClauseAllocator() { }

    inline void* allocate(unsigned int length) {
        unsigned int size = clauseBytes(length);
        if (!pages.back().hasMemory(size)) {
            pages.emplace_back(PAGE_SIZE);
        }
        return pages.back().getMemory(size);
    }

    inline void deallocate(Clause* clause) {
        if (contains(clause)) { 
            // in parallel scenario it is important not to delete clauses of the commonly used allocator ...
            clause->setDeleted();
        }
        else {
            // ... keep them to take care of them during synchronization
            deleted.push_back(clause);
        }
    }

    inline bool contains(Clause* clause) {
        for (Page& page : pages) {
            if (page.contains(clause)) {
                return true;
            }
        }
        return false;
    }

    std::vector<Clause*> reallocate() {
        std::vector<Clause*> clauses {};
        std::vector<Page> old_pages;
        old_pages.swap(pages);
        size_t page_size = PAGE_SIZE;
        for (Page& page : pages) {
            page_size += page.page_size;
        }
        pages.emplace_back(page_size);
        for (Page& old_page : old_pages) {
            unsigned int num_literals = 0;
            unsigned int num_bytes = 0;
            for (unsigned char *p = old_page.memory, *end = old_page.memory + old_page.cursor; p < end; p += num_bytes) {
                Clause* old_clause = (Clause*)p;
                num_literals = old_clause->size();
                num_bytes = clauseBytes(num_literals);
                if (!old_clause->isDeleted()) {
                    void* clause = allocate(num_literals);
                    memcpy(clause, (void*)old_clause, num_bytes);
                    clauses.push_back((Clause*)clause);
                }
            }
        }
        old_pages.clear();
        return clauses;
    }

    std::vector<Clause*> collect() {
        std::vector<Clause*> clauses {};
        for (Page& page : pages) {
            unsigned int num_bytes = 0;
            for (unsigned char *p = page.memory, *end = page.memory + page.cursor; p < end; p += num_bytes) {
                Clause* clause = (Clause*)p;
                if (!clause->isDeleted()) {
                    clauses.push_back(clause);
                }
                num_bytes = clauseBytes(clause->size());
            }
        }
        return clauses;
    }

    void copy(ClauseAllocator& other) {
        pages.clear();
        deleted.clear();
        size_t page_size = PAGE_SIZE;
        for (Page& page : other.pages) {
            page_size += page.page_size;
        }
        pages.emplace_back(page_size);
        for (const Page& page : other.pages) {
            unsigned int num_literals = 0;
            unsigned int num_bytes = 0;
            for (unsigned char *p = page.memory, *end = page.memory + page.cursor; p < end; p += num_bytes) {
                Clause* clause = (Clause*)p;
                num_literals = clause->size();
                num_bytes = clauseBytes(num_literals);
                if (!clause->isDeleted()) {
                    void* clause = allocate(num_literals);
                    memcpy(clause, (void*)clause, num_bytes);
                }
            }
        }
    }

    void absorb(ClauseAllocator& other) {
        for (Page& page : other.pages) {
            pages.emplace_back(std::move(page));
        }
        other.pages.clear();
        for (Clause* clause : other.deleted) {
            deallocate(clause);
        }
        other.deleted.clear();
        other.pages.emplace_back(PAGE_SIZE);
    }

private:
    const unsigned int PAGE_SIZE = 128*1024*1024;

    std::vector<Page> pages;
    std::vector<Clause*> deleted;

    void operator=(ClauseAllocator const&) = delete;

    inline unsigned int clauseBytes(unsigned int length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
