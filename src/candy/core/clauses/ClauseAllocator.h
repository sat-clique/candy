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
#include <candy/core/clauses/ClauseAllocatorPage.h>
#include <candy/core/Statistics.h>

namespace Candy {

class ClauseAllocator {
public:
    ClauseAllocator() : pages(), deleted() {
        pages.emplace_back(PAGE_SIZE);
    }

    ~ClauseAllocator() { }

    inline void* allocate(unsigned int length) {
        if (!pages.back().hasMemory(length)) {
            pages.emplace_back(PAGE_SIZE);
        }
        return pages.back().getMemory(length);
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
        for (ClauseAllocatorPage& page : pages) { 
            if (page.contains(clause)) {
                return true;
            }
        }
        return false;
    }

    inline size_t size() {
        size_t size = 0;
        for (ClauseAllocatorPage& page : pages) {
            size += page.size();
        }
        return size;
    }

    std::vector<Clause*> reallocate() {
        std::vector<Clause*> clauses {};
        std::vector<ClauseAllocatorPage> old_pages;
        old_pages.swap(pages);
        pages.emplace_back(size() + PAGE_SIZE);
        for (ClauseAllocatorPage& old_page : old_pages) {
            for (const Clause* old_clause : old_page) {
                if (!old_clause->isDeleted()) {
                    void* clause = allocate(old_clause->size());
                    memcpy(clause, (void*)old_clause, old_page.clauseBytes(old_clause->size()));
                    clauses.push_back((Clause*)clause);
                }
            }
        }
        old_pages.clear();
        return clauses;
    }

    std::vector<Clause*> collect() {
        std::vector<Clause*> clauses {};
        for (ClauseAllocatorPage& page : pages) {
            for (const Clause* clause : page) {
                if (!clause->isDeleted()) {
                    clauses.push_back((Clause*)clause);
                }
            }
        }
        return clauses;
    }

    void copy(ClauseAllocator& other) {
        pages.clear();
        deleted.clear();
        size_t page_size = size() + PAGE_SIZE;
        pages.emplace_back(page_size);
        for (const ClauseAllocatorPage& page : other.pages) {
            for (const Clause* clause : page) {
                if (!clause->isDeleted()) {
                    void* new_clause = allocate(clause->size());
                    memcpy(new_clause, (void*)clause, page.clauseBytes(clause->size()));
                }
            }
        }
    }

    void absorb(ClauseAllocator& other) {
        for (ClauseAllocatorPage& page : other.pages) {
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

    std::vector<ClauseAllocatorPage> pages;
    std::vector<Clause*> deleted;

    void operator=(ClauseAllocator const&) = delete;

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
