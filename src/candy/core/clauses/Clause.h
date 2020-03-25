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

#include <iostream>
#include <initializer_list>
#include <limits>
#include <algorithm>

#include "candy/core/SolverTypes.h"

#ifndef SRC_CANDY_CORE_CLAUSE_H_
#define SRC_CANDY_CORE_CLAUSE_H_

namespace Candy {

class Clause {
    friend class ClauseAllocatorMemory;
    friend class ClauseAllocator;
    friend class ClauseDatabase;
    friend class Subsumption;
    friend class Propagate;

private:
    uint16_t length;
    uint16_t weight;

    Lit literals[1];

    inline void swap(uint16_t pos1, uint16_t pos2) {
        assert(pos1 < length && pos2 < length);
        Lit tmp = literals[pos1];
        literals[pos1] = literals[pos2];
        literals[pos2] = tmp;
    }

    inline void setDeleted() {
        weight = std::numeric_limits<uint16_t>::max();
    }

    inline void setPersistent() {
        weight = 0;
    }

    inline void setLBD(uint16_t lbd) {
        weight = lbd;
    }

public:
    template<typename Iterator>
    Clause(Iterator begin, Iterator end, uint16_t lbd) {
        copyLiterals(begin, end, literals);
        length = static_cast<decltype(length)>(std::distance(begin, end));
        weight = lbd; // not frozen, not deleted and not learnt; lbd=0
        assert(lbd <= length);
    }
    
    Clause(std::initializer_list<Lit> list, uint16_t lbd = 0) : Clause(list.begin(), list.end(), lbd) { }

    ~Clause() { }

    void operator delete (void* p) = delete;

    inline const Lit& operator [](int i) const {
        return literals[i];
    }

    typedef const Lit* const_iterator;

    inline const_iterator begin() const {
        return literals;
    }

    inline const_iterator end() const {
        return literals + length;
    }

    inline uint16_t size() const {
        return length;
    }

    inline const Lit first() const {
        return literals[0];
    }

    inline const Lit second() const {
        return literals[1];
    }

    inline const Lit third() const {
        return literals[2];
    }

    inline const Lit back() const {
        return literals[length-1];
    }

    bool contains(const Lit lit) const {
        return std::find(begin(), end(), lit) != end();
    }

    void printDIMACS() const {
        for (Lit it : *this) {
            printLiteral(it);
            printf(" ");
        }
        printf("0\n");
    }

    void printDIMACS(std::vector<lbool> values) const {
        for (Lit it : *this) {
            printLiteral(it, values);
            printf(" ");
        }
        printf("0\n");
    }

    inline bool isPersistent() const {
        return weight == 0;
    }

    inline bool isLearnt() const {
        return weight > 0;
    }

    inline bool isDeleted() const {
        return weight == std::numeric_limits<uint16_t>::max();
    }

    inline uint16_t getLBD() const {
        return weight;
    }

private:
    template<typename InputIterator>
    inline void copyLiterals(InputIterator srcBegin, InputIterator srcEnd, Lit* target) {
        for(InputIterator srcIter = srcBegin; srcIter != srcEnd; ++srcIter) {
            *target = *srcIter;
            ++target;
        }
    }
};

inline std::ostream& operator <<(std::ostream& stream, Clause const& clause) {
    for (Lit lit : clause) {
        stream << lit << " ";
    }
    return stream;
}

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSE_H_ */
