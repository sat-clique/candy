/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Candy sources are based on Glucose which is based on MiniSat (see former copyrights below). 
Permissions and copyrights of Candy are exactly the same as Glucose and Minisat (see below).


--------------- Former Glucose Copyrights

 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
                                CRIL - Univ. Artois, France
                                LRI  - Univ. Paris Sud, France (2009-2013)
                                Labri - Univ. Bordeaux, France

Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it 
is based on. (see below).


--------------- Original Minisat Copyrights

Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

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
*************************************************************************************************/

#ifndef SRC_CANDY_SUBSUMPTION_H_

#include <unordered_map>
#include <vector>

#include "candy/core/simplification/SubsumptionClause.h"
#include "candy/core/simplification/SubsumptionClauseDatabase.h"

#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/core/propagate/Propagate.h"
#include "candy/mtl/Stamp.h"
#include "candy/frontend/CLIOptions.h"

namespace Candy {

class Subsumption { 
private:
    SubsumptionClauseDatabase& database;

    void unique(std::vector<SubsumptionClause*>& list);
    bool subsume(SubsumptionClause* clause);

public:   
    unsigned int nDuplicates;
    unsigned int nSubsumed;
    unsigned int nStrengthened;      

    Subsumption(SubsumptionClauseDatabase& database_)
     : database(database_), nDuplicates(0), nSubsumed(0), nStrengthened(0)
    { }

    bool subsume() {
        nDuplicates = nSubsumed = nStrengthened = 0; 

        std::vector<SubsumptionClause*> list { database.begin(), database.end() };

        sort(list.begin(), list.end(), [](const SubsumptionClause* c1, const SubsumptionClause* c2) { 
            return c1->size() < c2->size() || (c1->size() == c2->size() && c1->get_abstraction() < c2->get_abstraction()); 
        });

        unique(list);
        
        for (SubsumptionClause* clause : list) {
            if (!clause->is_deleted()) {
                if (!subsume(clause)) {
                    return false;
                }
            }
        }
        
        return true;
    }

    unsigned int nTouched() {
        return nDuplicates + nSubsumed + nStrengthened; 
    }
    
}; 

void Subsumption::unique(std::vector<SubsumptionClause*>& list) { // remove duplicates
    for (auto it1 = list.begin(); it1 != list.end(); it1++) {
        SubsumptionClause* clause1 = *it1;
        if (clause1->is_deleted()) continue;
        for (auto it2 = it1+1; it2 != list.end(); it2++) {
            SubsumptionClause* clause2 = *it2;
            if (clause2->is_deleted()) continue;
            if (clause1->get_abstraction() != clause2->get_abstraction()) {
                break;
            }
            if (clause1->equals(clause2)) {
                nDuplicates++;
                if (clause1->lbd() > clause2->lbd() || (clause1->lbd() == clause2->lbd() && clause1->get_clause() > clause2->get_clause())) {
                    database.remove(clause1);
                    assert(clause1->is_deleted());
                }
                else {
                    database.remove(clause2);
                    assert(clause2->is_deleted());
                }
                break;
            }
        }
    }
}

bool Subsumption::subsume(SubsumptionClause* clause) {
    // Find best variable to scan:
    Var best = var(*std::min_element(clause->begin(), clause->end(), [this] (Lit l1, Lit l2) {
        return database.numOccurences(var(l1)) < database.numOccurences(var(l2));
    }));

    bool persist = false;
    // Search all candidates:
    const std::vector<SubsumptionClause*> occurences = database.copyOccurences(best);
    for (SubsumptionClause* occurence : occurences) {
        if (occurence != clause && !occurence->is_deleted()) {
            Lit l = clause->subsumes(occurence);

            if (l == lit_Undef) {
                nSubsumed++;
                if (clause->get_clause()->isLearnt() && !occurence->get_clause()->isLearnt()) {
                    persist = true;
                }
                database.remove(occurence);
            }
            else if (l != lit_Error) {
                nStrengthened++;   
                if (occurence->size() > 1) {
                    database.strengthen(occurence, ~l);
                }
                else {
                    return false;
                }
            }
        }
    }

    if (persist) {
        database.persist(clause);
    }

    return true;
}

}
#endif