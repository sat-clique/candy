/*
 * Clause.h
 *
 *  Created on: Feb 15, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CLAUSE_H_
#define SRC_CANDY_CORE_CLAUSE_H_

#include "candy/core/SolverTypes.h"
#include "candy/core/ClauseAllocator.h"
#include <iostream>

namespace Candy {

// bits 0..12
#define BITS_LBD 13
#define LBD_MASK (static_cast<uint16_t>(8191))
// bit 13
#define LEARNT_MASK (static_cast<uint16_t>(8192))
// bit 14
#define DELETED_MASK (static_cast<uint16_t>(16384))
// bit 15
#define UNFROZEN_MASK (static_cast<uint16_t>(32768))

class Clause {
    uint16_t length;
    uint16_t header;

    union {
        float act;
        uint32_t abs;
    } data;

    Lit literals[1];

private:
    void* operator new (std::size_t size) throw() { assert(size != size); return nullptr; };

protected:
    static ClauseAllocator* allocator;

public:
    Clause(const std::vector<Lit>& ps, bool learnt);
    Clause(std::initializer_list<Lit> list);
    ~Clause();

    void* operator new (std::size_t size, uint16_t length);
    void operator delete (void* p);

    static void* allocate(uint16_t length);
    static void deallocate(Clause* clause);

    typedef Lit* iterator;
    typedef const Lit* const_iterator;

    Lit& operator [](int i);
    Lit operator [](int i) const;

    /**
     * clauses are smaller if header is bigger or (if header is equal) activity is smaller
     * header is bigger if
     * - clauses are not frozen
     * - clauses are already marked as deleted
     * - clauses are learnt
     * - clauses have the bigger lbd
     */
//    friend bool operator <(const Clause& clause1, const Clause& clause2) {
//        return clause1.header > clause2.header || (clause1.header == clause2.header && clause1.data.act < clause2.data.act);
//    }
//
//    friend bool operator >(const Clause& clause1, const Clause& clause2) {
//        return clause2 < clause1;
//    }
//
//    friend bool operator <=(const Clause& clause1, const Clause& clause2) {
//        return !(clause1 > clause2);
//    }
//
//    friend bool operator >=(const Clause& clause1, const Clause& clause2) {
//        return !(clause1 < clause2);
//    }

    bool operator <(Clause& clause2) {
        return header > clause2.header || (header == clause2.header && data.act < clause2.data.act);
    }

    bool operator >(Clause& clause2) {
        Clause& clause1 = *this;
        return clause2 < clause1;
    }

    bool operator <=(Clause& clause2) {
        Clause& clause1 = *this;
        return !(clause1 > clause2);
    }

    bool operator >=(Clause& clause2) {
        Clause& clause1 = *this;
        return !(clause1 < clause2);
    }

    const_iterator begin() const;
    const_iterator end() const;
    iterator begin();
    iterator end();
    uint16_t size() const;

    void calcAbstraction();

    const Lit back() const;
    bool contains(const Lit lit) const;
    bool contains(const Var var) const;

    inline void swap(uint16_t pos1, uint16_t pos2) {
        assert(pos1 < length && pos2 < length);
        Lit tmp = literals[pos1];
        literals[pos1] = literals[pos2];
        literals[pos2] = tmp;
    }

    bool isLearnt() const;
    void setLearnt(bool learnt);
    bool isDeleted() const;
    void setDeleted();
    bool isFrozen() const;
    void setFrozen(bool flag);
    void setLBD(uint16_t i);
    uint16_t getLBD() const;

    float& activity();
    uint32_t abstraction() const;


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

        if (other.size() < size() || (data.abs & ~other.data.abs) != 0) {
            return Glucose::lit_Error;
        }

        Lit ret = Glucose::lit_Undef;

        for (Lit c : *this) {
            // search for c or ~c
            for (Lit d : other) {
                if (c == d) {
                    goto ok;
                }
                else if (ret == Glucose::lit_Undef && c == ~d) {
                    ret = c;
                    goto ok;
                }
            }
            // did not find it
            return Glucose::lit_Error;
            ok: ;
        }

        return ret;
    }

    inline void strengthen(Lit p) {
        if (std::remove(begin(), end(), p) != end()) {
            --length;
        }
        calcAbstraction();
    }

    static void printAlignment() {
        Clause clause({Glucose::lit_Undef});
        uint64_t start = (uint64_t)&clause;
        uint64_t header = (uint64_t)&(clause.header);
        uint64_t data = (uint64_t)&(clause.data);
        uint64_t length = (uint64_t)&(clause.length);
        uint64_t literals = (uint64_t)&(clause.literals);
        std::cout << "c Size of Clause: " << sizeof(Candy::Clause) << std::endl;
        std::cout << "c Length starts at " << length - start << std::endl;
        std::cout << "c Header starts at " << header - start << std::endl;
        std::cout << "c Data-union starts at " << data - start << std::endl;
        std::cout << "c Literals start at " << literals - start << std::endl;
    }
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSE_H_ */
