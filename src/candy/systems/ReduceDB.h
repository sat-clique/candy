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

#ifndef SRC_CANDY_SYS_REDUCE_DB_H_
#define SRC_CANDY_SYS_REDUCE_DB_H_

#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"

namespace Candy {

class ReduceDB {
    ClauseDatabase& clause_db;
    Trail& trail;

    size_t nReduced, nReduceCalls_;

    const unsigned int persistentLBD;
    const unsigned int volatileLBD;

    unsigned int nbclausesbeforereduce; // To know when it is time to reduce clause database
    unsigned int incReduceDB;

public:
    ReduceDB(ClauseDatabase& clause_db_, Trail& trail_)
     : clause_db(clause_db_), trail(trail_), nReduced(0), nReduceCalls_(1), 
        persistentLBD(ClauseDatabaseOptions::opt_persistent_lbd), 
        volatileLBD(ClauseDatabaseOptions::opt_volatile_lbd), 
        nbclausesbeforereduce(ClauseDatabaseOptions::opt_first_reduce_db),
        incReduceDB(ClauseDatabaseOptions::opt_inc_reduce_db) { }
    
    ~ReduceDB() { }

    size_t nReduceCalls() {
        return nReduceCalls_;
    }

    void clear() {
        nReduceCalls_ = 0;
        nReduced = 0;
    }

    void process_conflict() {
        for (Clause* clause : clause_db.result.involved_clauses) {
            if (clause->getLBD() > persistentLBD) {
                uint8_t lbd = trail.computeLBD(clause->begin(), clause->end());
                clause->setLBD(lbd);
                clause->incUsed();
            }
        }

    }

    /**
     * only call this method at decision level 0
     **/
    void reduce() { 
        assert(trail.decisionLevel() == 0);
        for (Clause* learnt : clause_db) {
            if (learnt->getLBD() > persistentLBD) {
                if (learnt->getLBD() < volatileLBD) {
                    if (learnt->decUsed() == 0) {
                        clause_db.removeClause(learnt);
                        ++nReduced;
                    }
                }
                else if (learnt->decUsed() <= 1) {
                    clause_db.removeClause(learnt);
                    ++nReduced;
                }
            }
        }
    }

    inline bool trigger_reduce() {
        if (nReduceCalls_ * nbclausesbeforereduce < clause_db.result.nConflicts) {
            nbclausesbeforereduce += incReduceDB;
            nReduceCalls_++;
            return true;
        } 
        else {
            return false;
        }
    }

    void print_stats() {
        printf("c nb ReduceDB           : %zu\n", nReduceCalls_);
        printf("c nb removed Clauses    : %zu\n", nReduced);
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_SYS_REDUCE_DB_H_ */
