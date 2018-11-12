/*
 * Clause.h
 *
 *  Created on: Feb 15, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CLAUSE_H_
#define SRC_CANDY_CORE_CLAUSE_H_

#include "candy/core/SolverTypes.h"
#include <candy/core/Statistics.h>
#include <iostream>
#include <initializer_list>

namespace Candy {

// bits 0..11
#define BITS_LBD 12
#define LBD_MASK (static_cast<uint16_t>(4095))

#define SELECTABLE_BIT 12
#define LEARNT_BIT 13
#define DELETED_BIT 14
#define FROZEN_BIT 15

class Clause {
    uint16_t length;
    uint16_t header;

    float activity_;

    Lit literals[1];

private:
    inline Lit& operator [](int i) {
        return literals[i];
    }

    typedef Lit* iterator;

    inline iterator begin() {
        return literals;
    }

    inline iterator end() {
        return literals + length;
    }

    inline void swap(uint16_t pos1, uint16_t pos2) {
        assert(pos1 < length && pos2 < length);
        Lit tmp = literals[pos1];
        literals[pos1] = literals[pos2];
        literals[pos2] = tmp;
    }

    inline void setLearnt(bool flag) {
        header = (header & ~(1 << LEARNT_BIT)) | ((flag ? 1 : 0) << LEARNT_BIT);
    }

    inline void setSelectable(bool flag) {
        header = (header & ~(1 << SELECTABLE_BIT)) | ((flag ? 1 : 0) << SELECTABLE_BIT);
    }

    inline void setFrozen(bool flag) {
        header = (header & ~(1 << FROZEN_BIT)) | ((flag ? 1 : 0) << FROZEN_BIT);
    }

    inline void setDeleted() {
        header |= 1 << DELETED_BIT;
    }

    inline void setLBD(uint16_t i) {
        uint16_t flags = header & ~LBD_MASK;
        header = std::min(i, LBD_MASK);
        header |= flags;
    }

    inline float& activity() {
        return activity_;
    }

    inline void strengthen(Lit p) {
        std::remove(begin(), end(), p);
        --length;
    }

    friend class ClauseDatabase;
    friend class Propagate;
    friend class ConflictAnalysis;
    friend class Trail;
    friend class Subsumption;
    friend class TestClauseFactory;

public:
    Clause(std::initializer_list<Lit> list) {
        copyLiterals(list.begin(), list.end(), literals);
        length = static_cast<decltype(length)>(list.size());
        header = 0; // all flags false; lbd=0
        activity_ = 0;
    }

    Clause(const std::vector<Lit>& list) {
        copyLiterals(list.begin(), list.end(), literals);
        length = static_cast<decltype(length)>(list.size());
        header = 0; // not frozen, not deleted and not learnt; lbd=0
        activity_ = 0;
    }

    ~Clause();

    //void* operator new (std::size_t size) = delete;
    void operator delete (void* p) = delete;

    inline const Lit& operator [](int i) const {
        return literals[i];
    }

    inline const bool operator ==(Clause clause) const {
        bool equal = (length == clause.length);
        for (unsigned int i = 0; i < length && equal; i++) {
            equal &= (literals[i] == clause[i]);
        }
        return equal;
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

    inline const Lit back() const {
        return literals[length-1];
    }

    bool contains(const Lit lit) const;
    bool contains(const Var var) const;

    void printDIMACS() const;
    void printDIMACS(std::vector<lbool> values) const;

    inline bool isLearnt() const {
        return (header >> LEARNT_BIT) & 1;
    }

    inline bool isSelectable() const {
        return (header >> SELECTABLE_BIT) & 1;
    }

    inline bool isFrozen() const {
        return (header >> FROZEN_BIT) & 1;
    }

    inline bool isDeleted() const {
        return (header >> DELETED_BIT) & 1;
    }

    inline uint16_t getLBD() const {
        return header & LBD_MASK;
    }

    inline uint16_t getHeader() const {
        return header;
    }

    inline float getActivity() const {
        return activity_;
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
    inline Lit subsumes(const Clause& other) const {
        if (other.size() < size()) {
            return lit_Error;
        }

        Lit ret = lit_Undef;

        for (Lit c : *this) {
            // search for c or ~c
            for (Lit d : other) {
                if (c == d) {
                    goto ok;
                }
                else if (ret == lit_Undef && c == ~d) {
                    ret = c;
                    goto ok;
                }
            }
            // did not find it
            return lit_Error;
            ok: ;
        }

        return ret;
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
    stream << std::endl; 
    return stream;
}

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSE_H_ */
