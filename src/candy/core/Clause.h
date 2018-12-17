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

class Clause {
    friend class ClauseDatabase;
    friend class Propagate;
    friend class ConflictAnalysis;

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

    inline void setPersistent() {
        weight = 0;
    }

    inline void setDeleted() {
        weight = std::numeric_limits<uint16_t>::max();
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
    }
    
    Clause(std::initializer_list<Lit> list, uint16_t lbd = 0) : Clause(list.begin(), list.end(), lbd) { }

    ~Clause();

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
        return weight > 0;
    }

    inline bool isDeleted() const {
        return weight == std::numeric_limits<uint16_t>::max();
    }

    inline uint16_t getLBD() const {
        return weight;
    }

    inline std::vector<Lit> except(Lit lit) const {
        std::vector<Lit> literals;
        for (Lit literal : *this) if (literal != lit) literals.push_back(literal);
        return literals;
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
