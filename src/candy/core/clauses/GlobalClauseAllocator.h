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

#ifndef SRC_CANDY_GlobalClauseAllocator_H_
#define SRC_CANDY_GlobalClauseAllocator_H_

#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <candy/core/clauses/Clause.h>
#include <candy/core/clauses/ClauseAllocator.h>
#include <candy/core/Statistics.h>

namespace Candy {

class GlobalClauseAllocator {
public:
    GlobalClauseAllocator() : allocator(), active(false), alock(), ready() { }
    ~GlobalClauseAllocator() { }

    inline void* allocate(unsigned int length) {
        alock.lock();
        void* mem = allocator[int(active)].allocate(length);
        alock.unlock();
        return mem;
    }

    inline void enroll() {
        alock.lock();
        ready[std::this_thread::get_id()] = false;
        alock.unlock();
    }

    inline void import(ClauseAllocator& other) {
        alock.lock();
        allocator[int(active)].import(other); 
        other.clear();
        ready[std::this_thread::get_id()] = true;

        bool everybody_ready = false;
        for (auto it : ready) {
            everybody_ready &= it.second;
        }

        if (everybody_ready) { // all threads use active allocator now
            //free the old one and copy and cleanup the new one 
            allocator[int(!active)].clear();
            allocator[int(!active)].import(allocator[int(active)]);
            active = !active; // swap active allocator
            for (auto it : ready) {
                ready[it.first] = false;
            }
        }
        alock.unlock();
    }

    inline std::vector<Clause*> collect() {
        alock.lock();
        std::vector<Clause*> clauses = allocator[int(active)].collect();
        alock.unlock();
        return clauses;
    }

private:
    std::array<ClauseAllocator, 2> allocator;
    bool active;

    std::mutex alock;
    std::unordered_map<std::thread::id, bool> ready;

    GlobalClauseAllocator(GlobalClauseAllocator const&) = delete;
    void operator=(GlobalClauseAllocator const&) = delete;

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
