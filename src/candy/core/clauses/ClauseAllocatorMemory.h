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

#ifndef SRC_CANDY_CORE_CLAUSEALLOCATORPAGE_H_
#define SRC_CANDY_CORE_CLAUSEALLOCATORPAGE_H_

#include <cstdlib> 
#include <cstring> 
#include <assert.h>
#include <memory.h>

#include <candy/core/clauses/Clause.h>
#include <candy/core/clauses/ClauseAllocator.h>

namespace Candy {

class ClauseAllocatorPage {
public:
    class const_iterator {
    public:
        const_iterator(unsigned char* begin_) : pos(begin_) { }

        inline const Clause* operator*() const {
            return (Clause*)pos;
        }

        inline const_iterator& operator++() {
            pos += clauseBytes(((Clause*)pos)->size());
            return *this;
        }

        inline bool operator!=(const const_iterator& other) const {
            return pos != other.pos;
        }

    private:
        unsigned char* pos;

        inline unsigned int clauseBytes(unsigned int length) {
            return (sizeof(Clause) + sizeof(Lit) * (length-1));
        }
    };

private:
    size_t page_size;
    size_t cursor;
    unsigned char* memory;

    ClauseAllocatorPage(ClauseAllocatorPage const&) = delete;
    void operator=(ClauseAllocatorPage const&) = delete;

public:
    ClauseAllocatorPage(size_t page_size_) : page_size(page_size_), cursor(0) {
        memory = (unsigned char*)std::malloc(page_size);
        // std::memset(memory, page_size, 0);
    }

    ClauseAllocatorPage(ClauseAllocatorPage&& other) : page_size(other.page_size), cursor(other.cursor), memory(other.memory) {
        other.page_size = 0;
        other.cursor = 0;
        other.memory = nullptr;
    }

    ~ClauseAllocatorPage() {
        if (memory != nullptr) {
            std::free((void*)memory);
            memory = nullptr;
        }
    }

    inline bool hasMemory(size_t length) const {
        assert(memory != nullptr);
        return cursor + clauseBytes(length) < page_size;
    }

    inline void* allocate(size_t length) {
        assert(memory != nullptr);
        void* result = memory + cursor;
        cursor += clauseBytes(length);
        return result;
    }

    inline const_iterator begin() const {
        return const_iterator(memory);
    }

    inline const_iterator end() const {
        return const_iterator(memory + cursor);
    }

    inline size_t capacity() const {
        return page_size;
    }

    inline size_t used() const {
        return cursor;
    }

    inline void reset() {
        cursor = 0;
    }

    inline bool contains(void* p) const {
        assert(memory != nullptr);
        return p >= (void*)memory && p < (void*)(memory + cursor);
    }

    inline size_t clauseBytes(size_t length) const {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }
};

class ClauseAllocatorMemory {
private:
    const unsigned int PAGE_SIZE = 32*1024*1024;

    std::vector<ClauseAllocatorPage> pages;
    std::vector<ClauseAllocatorPage> old_pages;

public:
    ClauseAllocatorMemory() : pages(), old_pages() { }
    ~ClauseAllocatorMemory() { }

    inline void* allocate(size_t length) {
        if (pages.size() == 0 || !pages.back().hasMemory(length)) { 
            pages.emplace_back(PAGE_SIZE); 
        }
        return pages.back().allocate(length);
    }

    inline bool contains(Clause* clause) {
        for (ClauseAllocatorPage& page : pages) { 
            if (page.contains(clause)) {
                return true;
            }
        }
        return false;
    }

    inline size_t used() {
        size_t size = 0;
        for (ClauseAllocatorPage& page : pages) {
            size += page.used();
        }
        return size;
    }

    std::vector<Clause*> collect() {
        std::vector<Clause*> clauses;
        for (ClauseAllocatorPage& page : pages) {
            for (const Clause* clause : page) {
                if (!clause->isDeleted()) {
                    clauses.push_back((Clause*)clause);
                }
            }
        }
        return clauses;
    }

    void reallocate() {
        size_t size = used(); 
        old_pages.swap(pages);
        pages.emplace_back(size);
        for (ClauseAllocatorPage& old_page : old_pages) {
            for (const Clause* old_clause : old_page) {
                if (!old_clause->isDeleted()) {
                    void* clause = allocate(old_clause->size());
                    memcpy(clause, (void*)old_clause, old_page.clauseBytes(old_clause->size()));
                }
            }
        }
    }

    void clear() {
        pages.clear();
    }

    void free_old_pages() {
        old_pages.clear();
    }

    void import(ClauseAllocatorMemory& other, unsigned int limit) {
        for (const ClauseAllocatorPage& page : other.pages) {
            for (const Clause* clause : page) {
                if (!clause->isDeleted() && clause->getLBD() < limit) {
                    void* new_clause = allocate(clause->size());
                    memcpy(new_clause, (void*)clause, page.clauseBytes(clause->size()));
                    ((Clause*)clause)->setDeleted(); 
                }
            }
        }
    }

    void absorb(ClauseAllocatorMemory& other) {
        for (ClauseAllocatorPage& page : other.pages) {
            pages.emplace_back(std::move(page));
        }
    }

};

}

#endif