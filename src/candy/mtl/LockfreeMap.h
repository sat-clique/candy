/*************************************************************************************************
LockfreeMap -- Copyright (c) 2020, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef Lockfree_Map
#define Lockfree_Map

#include <cstdlib>
#include <cstring> 
#include <atomic>
#include <mutex>
#include <memory>
#include <vector>

#include "candy/core/SolverTypes.h"

/**
 * T is the content type and must be integral
 * N elements per page
 * S sentinel element
 * B counter bits, assert B <= 16, N < 2^B
 * */
template<typename T = Lit, unsigned int N = 50, T S = lit_Undef, unsigned int B = 8>
class LockfreeMap {
public:
    class const_iterator {
        T* pos;
        T** cpe; // current page end

    public:
        const_iterator(T* mem) : pos(mem), cpe((T**)(mem + N)) { }
        ~const_iterator() { }

        inline const T operator * () { 
            assert(pos != nullptr);
            return *pos; 
        }

        inline const_iterator& operator ++ () { 
            ++pos; 
            if (pos == (T*)cpe) { // hop from cpe to next page begin
                pos = *cpe; 
                if (pos != nullptr) cpe = (T**)(pos + N); 
            }
            if (pos != nullptr && *pos == S) pos = nullptr;
            return *this; 
        }

        inline bool operator != (const const_iterator& other) {
            return pos != other.pos;    
        }

        inline bool operator == (const const_iterator& other) const {
            return !(*this != other);
        }
    };

    static inline unsigned int get_index(uintptr_t pos) {
        return pos & ((1 << B) - 1);
    }

    static inline T* get_page(uintptr_t pos) {
        return (T*)(pos >> B);
    }

private:    
    class LockfreeVector {
        T* memory;
        std::atomic<uintptr_t> pos;

        LockfreeVector(LockfreeVector const&) = delete;
        void operator=(LockfreeVector const&) = delete;
        LockfreeVector(LockfreeVector&& other) = delete;

        void set_next(T* page, T* next) {
            T** cpe = (T**)(page + N);
            *cpe = next; // glue the segments
        }

        T* new_page() {
            T* page = (T*)std::malloc(N * sizeof(T) + sizeof(T*));
            std::fill(page, page + N, S);
            set_next(page, nullptr);
            return page;
        }

    public:
        LockfreeVector() {
            memory = nullptr;
            pos.store((uintptr_t)N, std::memory_order_relaxed);
        }

        ~LockfreeVector() { 
            T* mem = memory;
            while (mem != nullptr) {
                memory = *(T**)(mem + N);
                free(mem);
                mem = memory;
            }
        }

        void push(T value) {
            assert(value != S);
            while (true) {
                uintptr_t cur = pos.load(std::memory_order_acquire);
                unsigned int i = get_index(cur);
                if (i <= N) { // block pos++ during realloc (busy-loop)
                    cur = pos.fetch_add(1, std::memory_order_acq_rel);
                    i = get_index(cur);
                    T* mem = get_page(cur);
                    if (i < N) { 
                        mem[i] = value;
                        return;
                    }
                    else if (i == N) { // all smaller pos are allocated
                        T* page = new_page();
                        if (mem != nullptr) set_next(mem, page);
                        else memory = page; // initialization
                        pos.store((uintptr_t)page << B, std::memory_order_acq_rel);
                    } // loop to construct first element in new page
                }
            }
        }

        inline const_iterator begin() const {
            return const_iterator((memory != nullptr && *memory != S) ? memory : nullptr);
        }

        inline const_iterator end() const {
            return const_iterator(nullptr);
        }
    };

    LockfreeVector* map; 
    const unsigned int size_;

    LockfreeMap(LockfreeMap const&) = delete;
    void operator=(LockfreeMap const&) = delete;
    LockfreeMap(LockfreeMap&& other) = delete;

public:
    LockfreeMap(unsigned int n) : size_(n) {
        map = new LockfreeVector[n];
    }

    ~LockfreeMap() { 
        delete[] map;
    }

    unsigned int size() const {
        return size_;
    }

    LockfreeVector& operator [] (T key) {
        return map[key];
    }

};

#endif