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
#include <candy/frontend/CLIOptions.h>

namespace Candy {

class ClauseAllocator {
public:
    ClauseAllocator() : 
        memory(32), facts(1), 
        global_database_size_bound(ParallelOptions::opt_static_database_size_bound),
        global_allocator(nullptr), memory_lock(), ready(), ready_lock() { }

    ~ClauseAllocator() { }

    inline void* allocate(unsigned int length, unsigned int lbd) {
        if (global_allocator != nullptr && lbd == 0) {
            global_allocator->memory_lock.lock();
            void* mem = global_allocator->allocate(length, lbd);
            global_allocator->memory_lock.unlock();
            return mem;
        } 
        else {
            if (length == 1) {
                return facts.allocate(1);
            } else {
                return memory.allocate(length);
            }
        }
    }

    inline void deallocate(Clause* clause) {
        clause->setDeleted();
    }

    inline void synchronize() {
        if (global_allocator != nullptr) {
            ClauseAllocatorMemory transit { 1 };
            transit.import(this->memory, global_database_size_bound);
            // std::cout << "c " << std::this_thread::get_id() << ": Global allocator imports " << transit.used()/1024 << "kb of pages and deletes " << deleted.size() << " clauses" << std::endl;
            global_allocator->memory_lock.lock();
            global_allocator->memory.absorb(transit);
            global_allocator->memory_lock.unlock(); 
        }
    }
    
    void clear() {
        memory.clear();
        facts.clear();
    }

    void reorganize() {
        memory.reallocate();
        memory.free_phase_out_pages();

        if (global_allocator != nullptr) {
            if (global_allocator->everybody_ready()) { // all threads use new pages now
                // std::cout << "c " << std::this_thread::get_id() << ": Everybody using new pages, free the old ones" << std::endl;
                //free the old one and copy and cleanup the new one 
                global_allocator->memory.free_phase_out_pages();
                global_allocator->memory_lock.lock();
                global_allocator->memory.reallocate();
                global_allocator->memory_lock.unlock();
            } 
        }
    }

    std::vector<Clause*> collect() {
        std::vector<Clause*> clauses = memory.collect();
        std::vector<Clause*> unit_clauses = collect_unit_clauses();
        clauses.insert(clauses.end(), unit_clauses.begin(), unit_clauses.end());

        if (global_allocator != nullptr) {
            global_allocator->memory_lock.lock();
            std::vector<Clause*> global_clauses = global_allocator->memory.collect();
            global_allocator->memory_lock.unlock();
            global_allocator->set_ready(true);
            clauses.insert(clauses.end(), global_clauses.begin(), global_clauses.end());
        }

        return clauses;
    }

    std::vector<Clause*> collect_unit_clauses() {
        if (global_allocator != nullptr) {
            // these can be read without lock
            return global_allocator->facts.collect();
        }
        else {
            return facts.collect();
        }
    }

    void set_global_allocator(ClauseAllocator* global_allocator_) {
        assert(global_allocator == nullptr);
        assert(global_allocator_ != nullptr);
        global_allocator = global_allocator_;
        global_allocator->set_ready(false);
    }

    ClauseAllocator* create_global_allocator() {
        assert(global_allocator == nullptr);
        global_allocator = new ClauseAllocator();
        global_allocator->memory.absorb(this->memory); 
        global_allocator->facts.absorb(this->facts);
        global_allocator->set_ready(true); 
        return global_allocator;
    }

private:
    ClauseAllocatorMemory memory;
    ClauseAllocatorMemory facts;

    const unsigned int global_database_size_bound;

    // global allocator for multi-threaded scenario    
    ClauseAllocator* global_allocator;
    std::mutex memory_lock;
    
    std::unordered_map<std::thread::id, bool> ready;
    std::mutex ready_lock;

    ClauseAllocator(ClauseAllocator const&) = delete;
    void operator=(ClauseAllocator const&) = delete;

    inline void set_ready(bool flag) {
        ready_lock.lock();
        // if (flag) std::cout << "c " << std::this_thread::get_id() << ": Ready using new pages" << std::endl;
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
