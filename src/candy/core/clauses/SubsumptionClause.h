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

namespace Candy {

class SubsumptionClause {
private:
    const Clause* clause;
    uint64_t abstraction;

public:
    SubsumptionClause(const Clause* clause_) : clause(clause_) {
        abstraction = clause->calc_abstraction();
    }

    ~SubsumptionClause() { }

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

    inline bool equals(const Clause* other) const {
        SubsumptionClause sub { other };
        return this->equals(sub); 
    }

    inline bool equals(SubsumptionClause& other) const {
        return this->size() == other.size()
         && this->abstraction == other.abstraction
         && this->subsumes(other) == lit_Undef;
    }

    inline Lit subsumes(const Clause* other) const {
        SubsumptionClause sub { other };
        return this->subsumes(sub); 
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
    inline Lit subsumes(SubsumptionClause& other) const {
        if (other.size() >= this->size() && (abstraction & ~(other.abstraction)) == 0) {
            Lit ret = lit_Undef;
            for (Lit c : *this) {
                for (Lit d : other) { // search for c or ~c
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