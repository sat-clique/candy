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

namespace Candy {

// bits 0..11
#define BITS_LBD 12
#define LBD_MASK (static_cast<uint16_t>(4095))
// bit 12
#define SELECTABLE_MASK (static_cast<uint16_t>(4096))
// bit 13
#define LEARNT_MASK (static_cast<uint16_t>(8192))
// bit 14
#define DELETED_MASK (static_cast<uint16_t>(16384))
// bit 15
#define FROZEN_MASK (static_cast<uint16_t>(32768))

class Clause {
    uint16_t length;
    uint16_t header;

    union {
        float activity;
        uint32_t abstraction;
    } data;

    Lit literals[1];

public:
    Clause(const std::vector<Lit>& list, uint16_t lbd) {
        std::copy(list.begin(), list.end(), literals);
        length = static_cast<decltype(length)>(list.size());
        header = 0;
        setLearnt(true); // only learnts have lbd
        setLBD(lbd);
        data.activity = 0;
        assert(std::unique(begin(), end()) == end());
    }

    Clause(std::initializer_list<Lit> list) {
        std::copy(list.begin(), list.end(), literals);
        length = static_cast<decltype(length)>(list.size());
        header = 0; // all flags false; lbd=0
        calcAbstraction();
    }

    Clause(const std::vector<Lit>& list) {
        std::copy(list.begin(), list.end(), literals);
        length = static_cast<decltype(length)>(list.size());
        header = 0; // not frozen, not deleted and not learnt; lbd=0
        calcAbstraction();
    }

    ~Clause();

    //void* operator new (std::size_t size) = delete;
    void operator delete (void* p) = delete;

    typedef Lit* iterator;
    typedef const Lit* const_iterator;

    inline Lit& operator [](int i) {
        return literals[i];
    }

    inline const Lit operator [](int i) const {
        return literals[i];
    }

    inline const_iterator begin() const {
        return literals;
    }

    inline const_iterator end() const {
        return literals + length;
    }

    inline iterator begin() {
        return literals;
    }

    inline iterator end() {
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

    void calcAbstraction();

    bool contains(const Lit lit) const;
    bool contains(const Var var) const;

    void print() const;
    void print(std::vector<lbool> values) const;

    inline void swap(uint16_t pos1, uint16_t pos2) {
        assert(pos1 < length && pos2 < length);
        Lit tmp = literals[pos1];
        literals[pos1] = literals[pos2];
        literals[pos2] = tmp;
    }

    inline bool isLearnt() const {
        return (header & LEARNT_MASK) != 0;
    }

    inline void setLearnt(bool learnt) {
        if (learnt) {
            header |= LEARNT_MASK;
        } else {
            header &= ~LEARNT_MASK;
        }
    }

    inline bool isSelectable() const {
        return (header & SELECTABLE_MASK) != 0;
    }

    inline void setSelectable(bool hasAssumptions) {
        if (hasAssumptions) {
            header |= SELECTABLE_MASK;
        } else {
            header &= ~SELECTABLE_MASK;
        }
    }

    inline bool isFrozen() const {
        return (header & FROZEN_MASK) != 0;
    }

    inline void setFrozen(bool learnt) {
        if (learnt) {
            header |= FROZEN_MASK;
        } else {
            header &= ~FROZEN_MASK;
        }
    }

    inline bool isDeleted() const {
        return (header & DELETED_MASK) != 0;
    }

    inline void setDeleted() {
        header |= DELETED_MASK;
    }

    inline uint16_t getLBD() const {
        return header & LBD_MASK;
    }

    inline void setLBD(uint16_t i) {
        uint16_t flags = header & ~LBD_MASK;
        header = std::min(i, LBD_MASK);
        header |= flags;
    }

    inline float& activity() {
        return data.activity;
    }

    inline uint16_t getHeader() const {
        return header;
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
        assert(!isLearnt());
        assert(!other.isLearnt());

        if (other.size() < size() || (data.abstraction & ~other.data.abstraction) != 0) {
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


    inline void strengthen(Lit p) {
        if (std::remove(begin(), end(), p) != end()) {
            Statistics::getInstance().allocatorStrengthenClause(length);
            --length;
        }
        calcAbstraction();
    }

    void blow(uint8_t offset) {//use only if you know what you are doing (only to be used after strengthen calls)
        length += offset;
    }
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSE_H_ */
