/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

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

    void clear() {
        pages.clear();
        deleted.clear();
    }

    void import(ClauseAllocator& other) {
        for (const ClauseAllocatorPage& page : other.pages) {
            for (const Clause* clause : page) {
                if (!clause->isDeleted()) {
                    void* new_clause = allocate(clause->size());
                    memcpy(new_clause, (void*)clause, page.clauseBytes(clause->size()));
                }
            }
        }
        for (Clause* clause : other.deleted) {
            deallocate(clause);
        }
    }

    inline void move(ClauseAllocator& other) {
        for (ClauseAllocatorPage& page : other.pages) {
            this->pages.emplace_back(std::move(page));
        }
        for (Clause* clause : other.deleted) {
            this->deallocate(clause);
        }
        other.clear();
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
