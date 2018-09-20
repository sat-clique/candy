/*
 * ClauseAllocator.h
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CLAUSEALLOCATOR_H_
#define SRC_CANDY_CORE_CLAUSEALLOCATOR_H_

#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <numeric>

#include <candy/core/Clause.h>
#include <candy/core/Statistics.h>

namespace Candy {

class ClauseAllocator {

private:
    static constexpr unsigned int INITIAL_PAGE_SIZE = 64*1024*1024;

    std::vector<unsigned char*> pages;

    size_t cursor;
    size_t page_size;
    unsigned char* memory;

    inline void newPage() {
        memory = (unsigned char*)std::malloc(page_size);
        pages.push_back(memory);
        cursor = 0;
    }

    inline unsigned int clauseBytes(unsigned int length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }

public:
    ClauseAllocator();
    ~ClauseAllocator();

    ClauseAllocator(ClauseAllocator const&) = delete;
    void operator=(ClauseAllocator const&)  = delete;

    inline void* allocate(unsigned int length) {
        unsigned int size = clauseBytes(length);
        if (cursor + size > page_size) {
            newPage();
        }
        void* clause = memory + cursor;
        cursor += size;
        return clause;
    }

    inline void deallocate(Clause* clause) { 
        // just keep it dangling until defrag does its job
    }

    std::vector<Clause*> defrag(std::vector<Clause*> keep);

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */
