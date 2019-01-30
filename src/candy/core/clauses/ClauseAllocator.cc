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
            pages.emplace_back(size() + PAGE_SIZE);
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
            global_allocator->absorb(*this);
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