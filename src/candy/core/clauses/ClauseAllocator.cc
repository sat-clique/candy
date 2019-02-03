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

#include <candy/core/clauses/GlobalClauseAllocator.h>
#include <candy/core/clauses/ClauseAllocator.h>

namespace Candy {
    void* ClauseAllocator::allocate_globally(unsigned int length) {
        return global_allocator->allocate(length);
    }

    void ClauseAllocator::reallocate() {
        if (global_allocator == nullptr) {
            std::vector<ClauseAllocatorPage> old_pages;
            old_pages.swap(pages);
            pages.emplace_back(size());
            for (ClauseAllocatorPage& old_page : old_pages) {
                for (const Clause* old_clause : old_page) {
                    if (!old_clause->isDeleted()) {
                        void* clause = allocate(old_clause->size());
                        memcpy(clause, (void*)old_clause, old_page.clauseBytes(old_clause->size()));
                    }
                }
            }
            old_pages.clear();
        }
        else {
            global_allocator->import(*this);
        }
    }

    std::vector<Clause*> ClauseAllocator::collect() {
        if (global_allocator == nullptr) {
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
        else {
            return global_allocator->collect();
        }
    }

    void ClauseAllocator::setGlobalClauseAllocator(GlobalClauseAllocator* global_allocator) {
        assert(this->global_allocator == nullptr);
        this->global_allocator = global_allocator;
        global_allocator->enroll();
    }
}