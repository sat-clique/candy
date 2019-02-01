/*
 * ClauseAllocatorPage.h
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

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
    friend class ClauseAllocator;

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
        std::memset(memory, 0, page_size);
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

    inline void* getMemory(size_t length) {
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

    inline size_t size() const {
        return page_size;
    }

    inline bool contains(void* p) const {
        assert(memory != nullptr);
        return p >= (void*)memory && p < (void*)(memory + cursor);
    }

    inline size_t clauseBytes(size_t length) const {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }
};

}

#endif