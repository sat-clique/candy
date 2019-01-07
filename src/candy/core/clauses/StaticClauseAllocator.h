/*
 * ClauseAllocator.h
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_STATICCLAUSEALLOCATOR_H_
#define SRC_CANDY_STATICCLAUSEALLOCATOR_H_

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

class StaticClauseAllocator {
public:
    StaticClauseAllocator() : mutex(), users() { }
    ~StaticClauseAllocator() { }

    inline void* allocate(unsigned int length) {
        mutex.lock();
        void* storage = allocator.allocate(length);
        mutex.unlock();
        return storage;
    }

    inline void free_old_pages() {
        bool free_old_pages = false;
        users[std::this_thread::get_id()] = true;
        for (auto it : users) {
            free_old_pages &= it.second;
        }
        if (free_old_pages) {
            mutex.lock();
            allocator.free_old_pages();
            mutex.unlock();
            for (auto it : users) {
                users[it.first] = false;
            }
        }
    }

    inline void reallocate() {
        mutex.lock();
        allocator.reallocate();
        mutex.unlock();
    }

    inline std::vector<Clause*> collect() {
        mutex.lock();
        std::vector<Clause*> clauses = allocator.collect();
        mutex.unlock();
        return clauses;
    }

    inline void enroll() {
        mutex.lock();
        users[std::this_thread::get_id()] = false;
        mutex.unlock();
    }

private:
    static ClauseAllocator allocator;

    std::mutex mutex;
    std::unordered_map<std::thread::id, bool> users;

    StaticClauseAllocator(StaticClauseAllocator const&) = delete;
    void operator=(StaticClauseAllocator const&) = delete;

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
