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
#include <bitset>

#include "candy/core/SolverTypes.h"

#ifndef SRC_CANDY_CORE_CLAUSE_H_
#define SRC_CANDY_CORE_CLAUSE_H_

namespace Candy {

template<typename T>
static uint8_t cast_uint8_t(T num) {
    if (num > std::numeric_limits<uint8_t>::max()) {
        return std::numeric_limits<uint8_t>::max()-1;
    }
    return (uint8_t)num;
}

template<unsigned int X, unsigned int Y, unsigned int Z>
class Propagation2WLX;

class Clause {
    // want to keep track of systems which use the (private) setters,
    // in case I ever have to handle concurrency issues again
    friend class ClauseAllocatorMemory;
    friend class ClauseAllocator;
    friend class ClauseDatabase;
    friend class Subsumption;
    friend class Propagation2WL;
    friend class Propagation2WLStable1W;
    friend class Propagation2WL3Full;
    friend class Propagation2WLX<3,1,2>;
    friend class Propagation2WLX<3,4,2>;
    friend class Propagation2WLX<3,4,5>;
    friend class ReduceDB;

private:
    uint16_t length;
    uint8_t weight;
    uint8_t used;

    uint32_t abstraction;

    Lit literals[1];

    inline void swap(uint16_t pos1, uint16_t pos2) {
        assert(pos1 < length && pos2 < length);
        Lit tmp = literals[pos1];
        literals[pos1] = literals[pos2];
        literals[pos2] = tmp;
    }

    inline void setPersistent() {
        weight = 0;
    }

    inline void setLBD(uint8_t lbd) {
        weight = lbd;
    }

    inline uint8_t incUsed() {
        return used < 3 ? ++used : used;
    }

    inline uint8_t decUsed() {
        return used > 0 ? --used : used;
    }

    inline void setDeleted() {
        weight = std::numeric_limits<uint8_t>::max();
    }

    inline void calc_abstraction() {
        abstraction = 0;
        for (Lit lit : *this) {
            abstraction |= 1ull << (lit.var() % 32);
        }
    }

public:
    template<typename T>
    inline void sort(std::vector<T>& o, bool asc) {
        if (asc) {
            std::sort(literals, literals + length, [&o](Lit lit1, Lit lit2) { return o[lit1] < o[lit2]; });
        }
        else {
            std::sort(literals, literals + length, [&o](Lit lit1, Lit lit2) { return o[lit1] > o[lit2]; });
        }
    }

    template<typename T>
    inline void sort2(std::vector<T>& o, bool asc) {
        if (asc) {
            std::sort(literals, literals + length, [&o](Lit lit1, Lit lit2) { return o[lit1] - o[~lit1] < o[lit2] - o[~lit2]; });
        }
        else {
            std::sort(literals, literals + length, [&o](Lit lit1, Lit lit2) { return o[lit1] - o[~lit1] > o[lit2] - o[~lit2]; });
        }
    }

    template<typename Iterator>
    Clause(Iterator begin, Iterator end, unsigned int lbd) {
        copyLiterals(begin, end, literals);
        length = static_cast<decltype(length)>(std::distance(begin, end));
        weight = cast_uint8_t(lbd); // not frozen, not deleted and not learnt; lbd=0
        used = 2;
        calc_abstraction();
        assert(lbd <= length);
    }
    
    Clause(std::initializer_list<Lit> list, unsigned int lbd = 0) : Clause(list.begin(), list.end(), lbd) { }

    ~Clause() { }

    void operator delete (void* p) = delete;

    inline const Lit operator [](int i) const {
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
        return weight == std::numeric_limits<uint8_t>::max();
    }

    inline uint8_t getLBD() const {
        return weight;
    }

    inline bool equals(const Clause* other) const {
        if (this->abstraction == other->abstraction && this->size() == other->size()) {
            for (Lit lit : *this) {
                if (!other->contains(lit)) {
                    return false;
                }
            }
            return true;
        } 
        return false;
    }

    /**
     *  subsumes : (other : const Clause&)  ->  Lit
     *
     *  Description:
     *       Checks if clause subsumes 'other', and at the same time, if it can be used to simplify 'other'
     *       by subsumption resolution.
     *
     *    Result:
     *       lit_Error  - No subsumption or simplification
     *       lit_Undef  - Clause subsumes 'other'
     *       p          - The literal p can be deleted from 'other'
     */
    inline Lit subsumes(const Clause* other) const {
        if ((abstraction & ~(other->abstraction)) == 0 && other->size() >= this->size()) {
            Lit ret = lit_Undef;
            for (Lit c : *this) {
                for (Lit d : *other) { // search for c or ~c
                    if (c == d) {
                        goto ok;
                    }
                    else if (ret == lit_Undef && c == ~d) {
                        ret = c;
                        goto ok;
                    }
                }
                return lit_Error; // did not find it
                ok: ;
            }
            return ret;
        }
        else {
            return lit_Error;
        }
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
