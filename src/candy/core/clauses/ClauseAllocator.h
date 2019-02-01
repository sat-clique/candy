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

class GlobalClauseAllocator;

class ClauseAllocator {
public:
    ClauseAllocator() : pages(), deleted(), global_allocator(nullptr) { }

    ~ClauseAllocator() { }

    inline void* allocate(unsigned int length) {
        if (global_allocator == nullptr || length > 1) {
            if (pages.size() == 0 || !pages.back().hasMemory(length)) {
                pages.emplace_back(PAGE_SIZE);
            }
            return pages.back().getMemory(length);
        }
        else {
            return allocate_globally(1);
        } 
    }

    inline void deallocate(Clause* clause) {
        if (global_allocator == nullptr) {
            clause->setDeleted();
        }
        else {
            if (this->contains(clause)) { 
                // in parallel scenario it is important not to delete clauses of the commonly used allocator ...
                clause->setDeleted();
            }
            else {
                // ... keep them to take care of them during synchronization
                deleted.push_back(clause);
            }
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
    }

    void reallocate();
    std::vector<Clause*> collect();

    void setGlobalClauseAllocator(GlobalClauseAllocator* global_allocator);

private:
    const unsigned int PAGE_SIZE = 128*1024*1024;

    std::vector<ClauseAllocatorPage> pages;
    std::vector<Clause*> deleted;

    GlobalClauseAllocator* global_allocator;

    void operator=(ClauseAllocator const&) = delete;

    void* allocate_globally(unsigned int length);

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
