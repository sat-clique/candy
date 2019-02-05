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
#include <mutex>
#include <thread>
#include <unordered_map>

#include <candy/core/clauses/Clause.h>
#include <candy/core/clauses/ClauseAllocatorPage.h>
#include <candy/core/Statistics.h>

namespace Candy {

class ClauseAllocator {
public:
    ClauseAllocator() : pages(), old_pages(), deleted(), global_allocator(nullptr), alock(), ready(), ready_lock() { }

    ~ClauseAllocator() { }

    inline void* allocate(unsigned int length) {
        if (global_allocator == nullptr || length > 1) {
            if (pages.size() == 0 || !pages.back().hasMemory(length)) {
                pages.emplace_back(PAGE_SIZE);
            }
            return pages.back().getMemory(length);
        }
        else {
            assert(length == 1);
            global_allocator->lock();
            void* mem = global_allocator->allocate(1);
            global_allocator->unlock();
            return mem;
        } 
    }

    inline void deallocate(Clause* clause) {
        if (global_allocator == nullptr || this->contains(clause)) { 
            clause->setDeleted();
        }
        else { 
            // in parallel scenario it is important not to delete clauses of the commonly used allocator before synchronization happens
            // instead keep them to take care of them during synchronization
            deleted.push_back(clause);
        }
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

    void reorganize() {
        if (global_allocator == nullptr) {
            reallocate();
            old_pages.clear();
        }
        else {
            std::cout << "c global allocator imports pages of size " << used_space() << " from: " << std::this_thread::get_id() << std::endl;
            global_allocator->lock();
            for (ClauseAllocatorPage& page : this->pages) {
                global_allocator->pages.emplace_back(std::move(page));
            }
            std::cout << "c global allocator deletes " << deleted.size() << " clauses from: " << std::this_thread::get_id() << std::endl;
            for (Clause* clause : this->deleted) {
                global_allocator->deallocate(clause);
            }
            global_allocator->unlock(); 
            pages.clear();
            deleted.clear();
            assert(used_space() == 0);

            if (global_allocator->is_ready()) { // all threads use new pages now
                std::cout << "c global allocator reorganization" << std::endl;
                //free the old one and copy and cleanup the new one 
                global_allocator->old_pages.clear();
                global_allocator->lock();
                global_allocator->reallocate();
                global_allocator->unlock(); 
            }
        }
    }

    std::vector<Clause*> collect() {
        std::vector<Clause*> clauses {};
        if (global_allocator != nullptr) {
            global_allocator->lock();
            clauses = global_allocator->collect();
            global_allocator->unlock();
            global_allocator->set_ready();
        }
        for (ClauseAllocatorPage& page : pages) {
            for (const Clause* clause : page) {
                if (!clause->isDeleted()) {
                    clauses.push_back((Clause*)clause);
                }
            }
        }
        return clauses;
    }

    void set_global_allocator(ClauseAllocator* global_allocator_) {
        assert(global_allocator == nullptr);
        std::cout << "c thread registration in global allocator: " << std::this_thread::get_id() << std::endl;
        global_allocator = global_allocator_;
        global_allocator->lock();
        global_allocator->ready[std::this_thread::get_id()] = false;
        global_allocator->unlock();
    }

    ClauseAllocator* create_global_allocator() {
        assert(global_allocator == nullptr);
        global_allocator = new ClauseAllocator();
        global_allocator->lock();
        global_allocator->ready[std::this_thread::get_id()] = false;
        for (ClauseAllocatorPage& page : this->pages) {
            global_allocator->pages.emplace_back(std::move(page));
        }
        global_allocator->unlock();
        pages.clear();
        deleted.clear();
        return global_allocator;
    }

private:
    const unsigned int PAGE_SIZE = 32*1024*1024;

    std::vector<ClauseAllocatorPage> pages;
    std::vector<ClauseAllocatorPage> old_pages;
    
    std::vector<Clause*> deleted;

    // global allocator for multi-threaded scenario    
    ClauseAllocator* global_allocator;
    std::mutex alock;
    std::unordered_map<std::thread::id, bool> ready;
    std::mutex ready_lock;

    ClauseAllocator(ClauseAllocator const&) = delete;
    void operator=(ClauseAllocator const&) = delete;

    inline void lock() {
        alock.lock();
    }

    inline void unlock() {
        alock.unlock();
    }

    inline void set_ready() {
        ready_lock.lock();
        ready[std::this_thread::get_id()] = true;
        ready_lock.unlock();
    }

    inline bool is_ready() {
        ready_lock.lock();
        bool everybody_ready = true;
        for (auto it : ready) {
            everybody_ready &= it.second;
        }
        if (everybody_ready) {
            for (auto it : ready) {
                ready[it.first] = false;
            }
        }
        ready_lock.unlock();
        return everybody_ready;
    }

    inline bool contains(Clause* clause) {
        for (ClauseAllocatorPage& page : pages) { 
            if (page.contains(clause)) {
                return true;
            }
        }
        return false;
    }

    inline size_t used_space() {
        size_t size = 0;
        for (ClauseAllocatorPage& page : pages) {
            size += page.fill();
        }
        return size;
    }

    void reallocate() {
        size_t size = used_space(); 
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

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
