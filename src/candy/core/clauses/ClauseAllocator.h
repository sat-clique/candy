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
#include <candy/core/clauses/ClauseAllocatorMemory.h>
#include <candy/core/Statistics.h>
#include <candy/frontend/CLIOptions.h>

namespace Candy {

class ClauseAllocator {
public:
    ClauseAllocator() : 
        memory(32), facts(1), deleted(), 
        global_database_lbd_upper_bound(ParallelOptions::opt_lbd_static_database),
        global_allocator(nullptr), alock(), ready(), ready_lock() { }

    ~ClauseAllocator() { }

    inline void* allocate(unsigned int length) {
        if (global_allocator == nullptr || length > 1) {
            if (length == 1) {
                return facts.allocate(1);
            } else {
                return memory.allocate(length);
            }
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
        if (global_allocator == nullptr || memory.contains(clause)) { 
            clause->setDeleted();
        }
        else { 
            // in parallel scenario it is important not to delete clauses of the commonly used allocator before synchronization happens
            // instead keep them to take care of them during synchronization
            deleted.push_back(clause);
        }
    }

    void reorganize() {
        if (global_allocator == nullptr) {
            memory.reallocate();
            memory.free_old_pages();
        }
        else {
            std::cout << "c " << std::this_thread::get_id() << ": global allocator imports " << memory.used()/(1024*1024) << "MB of pages and deletes " << deleted.size() << " clauses" << std::endl;
            global_allocator->lock();
            global_allocator->memory.import(this->memory, global_database_lbd_upper_bound);
            for (Clause* clause : this->deleted) {
                global_allocator->deallocate(clause);
            }
            global_allocator->unlock(); 
            memory.reallocate();
            memory.free_old_pages();
            deleted.clear();

            if (global_allocator->everybody_ready()) { // all threads use new pages now
                //free the old one and copy and cleanup the new one 
                global_allocator->memory.free_old_pages();
                global_allocator->lock();
                global_allocator->memory.reallocate();
                global_allocator->unlock(); 
            } 
        }
    }

    std::vector<Clause*> collect() {
        std::vector<Clause*> clauses = memory.collect();
        std::vector<Clause*> unit_clauses = facts.collect();
        clauses.insert(clauses.end(), unit_clauses.begin(), unit_clauses.end());
        if (global_allocator != nullptr) {
            global_allocator->lock();
            std::vector<Clause*> global_clauses = global_allocator->collect();
            global_allocator->unlock();
            global_allocator->set_ready(true);
            clauses.insert(clauses.end(), global_clauses.begin(), global_clauses.end());
        }
        return clauses;
    }

    std::vector<Clause*> collect_unit_clauses() {
        std::vector<Clause*> unit_clauses = facts.collect();
        if (global_allocator != nullptr) {
            global_allocator->lock();
            std::vector<Clause*> global_unit_clauses = global_allocator->collect_unit_clauses();
            global_allocator->unlock();
            unit_clauses.insert(unit_clauses.end(), global_unit_clauses.begin(), global_unit_clauses.end());
        }
        return unit_clauses;
    }

    void set_global_allocator(ClauseAllocator* global_allocator_) {
        assert(global_allocator == nullptr);
        std::cout << "c thread registration in global allocator: " << std::this_thread::get_id() << std::endl;
        global_allocator = global_allocator_;
        global_allocator->set_ready(false);
    }

    ClauseAllocator* create_global_allocator() {
        assert(global_allocator == nullptr);
        global_allocator = new ClauseAllocator();
        global_allocator->set_ready(false);
        global_allocator->lock();
        global_allocator->memory.absorb(this->memory); 
        global_allocator->facts.absorb(this->facts); 
        global_allocator->unlock();
        deleted.clear();
        return global_allocator;
    }

private:
    ClauseAllocatorMemory memory;
    ClauseAllocatorMemory facts;
    
    std::vector<Clause*> deleted;

    const unsigned int global_database_lbd_upper_bound;

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

    inline void set_ready(bool flag) {
        ready_lock.lock();
        ready[std::this_thread::get_id()] = flag;
        ready_lock.unlock();
    }

    inline bool everybody_ready() {
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

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
