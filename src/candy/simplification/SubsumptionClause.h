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

#ifndef SRC_CANDY_CORE_SUBSUMPTION_CLAUSE_H_
#define SRC_CANDY_CORE_SUBSUMPTION_CLAUSE_H_

#include "candy/core/clauses/Clause.h"
#include <tuple>

namespace Candy {

class SubsumptionClause {
private:
    uint64_t abstraction;
    const Clause* clause;

    inline void calc_abstraction() {
        abstraction = 0;
        for (Lit lit : *clause) {
            abstraction |= 1ull << (lit.var() % 64);
        }
    }

public:
    SubsumptionClause(const Clause* clause_) : clause(clause_) {
        calc_abstraction();
    }

    ~SubsumptionClause() { }

    inline void reset(const Clause* clause_) {
        clause = clause_;
        calc_abstraction();
    }

    inline void operator =(const SubsumptionClause& other) {
        clause = other.clause;
        abstraction = other.abstraction;
    }

    inline const Lit& operator [](int i) const {
        return (*clause)[i];
    }

    typedef const Lit* const_iterator;

    inline const_iterator begin() const {
        return clause->begin();
    }

    inline const_iterator end() const {
        return clause->end();
    }

    inline uint16_t size() const {
        if (clause == nullptr) return 0;
        return clause->size();
    }

    inline uint16_t lbd() const {
        return clause->getLBD();
    }

    inline Clause* get_clause() const {
        return (Clause*)clause;
    }

    inline uint64_t get_abstraction() const {
        return abstraction;
    }

    inline void set_deleted() {
        this->clause = nullptr;
    }

    inline bool is_deleted() const {
        return this->clause == nullptr;
    }

    inline bool contains(Lit lit) const {
        return clause->contains(lit);
    }

    inline bool equals(const SubsumptionClause* other) const {
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
    inline Lit subsumes(const SubsumptionClause* other) const {
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

};

} /* namespace Candy */

#endif
