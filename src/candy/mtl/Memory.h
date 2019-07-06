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

#ifndef SRC_CANDY_CORE_MEMORY_H_
#define SRC_CANDY_CORE_MEMORY_H_

#include <cstdlib> 
#include <cstring> 
#include <assert.h>
#include <memory.h>
#include <vector>
#include <algorithm>

namespace Candy {

template<class T> 
class MemoryPage {
public:
    class const_iterator {
    public:
        const_iterator(unsigned char* begin_) : pos(begin_) { }

        inline const T* operator * () const {
            return (T*)pos;
        }

        inline const_iterator& operator ++ () {
            pos += sizeof(T);
            return *this;
        }

        inline bool operator == (const const_iterator& other) const {
            return pos == other.pos;
        }

        inline bool operator != (const const_iterator& other) const {
            return pos != other.pos;
        }

        inline bool operator < (const const_iterator& other) const {
            return pos < other.pos;
        }

    private:
        unsigned char* pos;

    };

private:
    size_t page_size;
    size_t cursor;
    unsigned char* memory;

    MemoryPage(MemoryPage const&) = delete;
    void operator=(MemoryPage const&) = delete;

public:
    MemoryPage(size_t page_size_) : page_size(page_size_), cursor(0) {
        memory = (unsigned char*)std::malloc(page_size);
        // std::memset(memory, page_size, 0);
    }

    MemoryPage(MemoryPage&& other) : page_size(other.page_size), cursor(other.cursor), memory(other.memory) {
        other.page_size = 0;
        other.cursor = 0;
        other.memory = nullptr;
    }

    ~MemoryPage() {
        if (memory != nullptr) {
            std::free((void*)memory);
            memory = nullptr;
        }
    }

    inline bool hasMemory() const {
        assert(memory != nullptr);
        return cursor + sizeof(T) < page_size;
    }

    inline void* allocate() {
        assert(memory != nullptr);
        void* result = memory + cursor;
        cursor += sizeof(T);
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

    template<class Compare>
    inline void sort(Compare comp) {
        std::sort((T*)memory, (T*)(memory + cursor), comp); 
    }

};

/**
 * class Memory:
 * 
 *  Manages a list of MemoryPages
 * 
 * */
template<class T>
class Memory {
public:
    class const_iterator {
    public:
        const_iterator(
            typename std::vector<MemoryPage<T>>::const_iterator page_,
            typename std::vector<MemoryPage<T>>::const_iterator end_page_,
            typename MemoryPage<T>::const_iterator position_,
            typename MemoryPage<T>::const_iterator end_position_)
         : page(page_), end_page(end_page_), position(position_), end_position(end_position_) 
        { 
            skip_empty_pages();
        } 

        inline const T* operator * () const {
            if (position == nullptr) return nullptr; 
            return *position;
        }

        inline const T* operator -> () const {
            if (position == nullptr) return nullptr; 
            return *position;
        }

        inline const_iterator& operator ++ () {
            skip_empty_pages();

            if (position != nullptr) {
                ++position;
                skip_empty_pages();
            }

            return *this;
        }

        inline const_iterator operator + (unsigned int num) {
            const_iterator iter = const_iterator(page, end_page, position, end_position);
            for (unsigned int i = 0; i < num; ++i) {
                ++iter;
            }
            return iter;
        }

        inline bool operator == (const const_iterator& other) const {
            return page == other.page && position == other.position;
        }

        inline bool operator != (const const_iterator& other) const {
            return page != other.page || position != other.position;
        }

        inline bool operator < (const const_iterator& other) const {
            return page < other.page || position < other.position;
        }

    private:
        typename std::vector<MemoryPage<T>>::const_iterator page;
        typename std::vector<MemoryPage<T>>::const_iterator end_page;
        typename MemoryPage<T>::const_iterator position;
        typename MemoryPage<T>::const_iterator end_position;

        inline void skip_empty_pages() {
            while (position == end_position && page != end_page) {
                page++;
                if (page != end_page) {
                    position = page->begin();
                    end_position = page->end();
                } else {
                    position = nullptr;
                    end_position = nullptr;
                }
            }
        }

    };

private:
    const unsigned int default_page_size;

    std::vector<MemoryPage<T>> pages;

public:
    Memory(unsigned int page_size_mb = 32)
     : default_page_size(page_size_mb*1024*1024), pages() { }
    ~Memory() { }

    inline const_iterator begin() const {
        if (pages.size() == 0) return end();
        return const_iterator(pages.begin(), pages.end(), pages.begin()->begin(), pages.begin()->end());
    }

    inline const_iterator end() const {
        return const_iterator(pages.end(), pages.end(), nullptr, nullptr);
    }

    inline void* allocate() {
        if (pages.size() == 0 || !pages.back().hasMemory()) { 
            pages.emplace_back(default_page_size); 
        }
        return pages.back().allocate();
    }

    void free_all() {
        pages.clear();
    }

    inline size_t used() {
        size_t size = 0;
        for (MemoryPage<T>& page : pages) {
            size += page.used();
        }
        return size;
    }

    template<class Compare>
    inline void sort(Compare comp) {
        if (pages.size() > 0) {
            if (pages.size() > 1) {
                std::vector<MemoryPage<T>> new_pages;
                new_pages.emplace_back(this->used());
                MemoryPage<T>& page = new_pages.back();
                for (const T* element : *this) {
                    void* new_element = page.allocate();
                    memcpy(new_element, (void*)element, sizeof(T));
                }
                pages.swap(new_pages);
            }
            pages.back().sort(comp);
        }
    }

};

}

#endif