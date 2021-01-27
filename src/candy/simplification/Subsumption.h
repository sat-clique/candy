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

#include "candy/simplification/OccurenceList.h"

#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/mtl/Stamp.h"
#include "candy/utils/CLIOptions.h"

namespace Candy {

class Subsumption { 
private:
    ClauseDatabase& clause_db;
    OccurenceList& occurences;

    bool subsume(Clause* clause);
    bool unit_resolution(Clause* clause);

public:
    unsigned int nDuplicates;
    unsigned int nSubsumed;
    unsigned int nStrengthened;      

    Subsumption(ClauseDatabase& clause_db_, OccurenceList& occurences_)
     : clause_db(clause_db_), occurences(occurences_), nDuplicates(0), nSubsumed(0), nStrengthened(0)
    { }

    bool subsume() {
        nDuplicates = nSubsumed = nStrengthened = 0; 
        
        for (unsigned int i = 0; i < clause_db.size(); i++) {
            Clause* clause = clause_db[i];
            if (!clause->isDeleted()) {
                // std::cout << "c Subsumption with clause " << *clause << "\r";
                if (clause->size() == 1) {
                    if (!unit_resolution((Clause*)clause)) {
                        return false;
                    }
                }
                else {
                    if (!subsume((Clause*)clause)) {
                        return false;
                    }
                }
            }
        }
        
        return true;
    }

    unsigned int nTouched() {
        return nSubsumed + nStrengthened; 
    }
    
}; 

bool Subsumption::unit_resolution(Clause* clause) {
    assert(clause->size() == 1);
    Lit unit = clause->first();
    for (Clause* occurence : occurences[unit.var()]) {
        if (occurence != clause && !occurence->isDeleted()) {
            if (occurence->contains(unit)) {
                // std::cout << "c " << unit << " - unit resolution removing clause " << *occurence << std::endl;
                clause_db.removeClause(occurence);
            } else {
                if (occurence->size() == 1) {
                    return false;
                }
                // std::cout << "c " << unit << " - unit resolution strengthening clause " << *occurence << std::endl;
                Clause* clause = clause_db.strengthenClause(occurence, ~unit);
                occurences.add(clause);
            }
        }
    }
    return true;
}

bool Subsumption::subsume(Clause* clause) {
    // Find best variable to scan:
    Lit best = *std::min_element(clause->begin(), clause->end(), [this] (Lit l1, Lit l2) {
        return occurences.count(l1.var()) < occurences.count(l2.var());
    });

    // Search all candidates:
    std::vector<Clause*> list = occurences.copy(best.var());
    for (Clause* occurence : list) {
        if (occurence != clause && !occurence->isDeleted()) {
            Lit l = clause->subsumes(occurence);

            if (l == lit_Undef) { 
                if (clause->size() == occurence->size()) {
                    nDuplicates++;
                } else {
                    nSubsumed++;
                }
                clause->setPersistent();
                clause_db.removeClause(occurence);
            }
            else if (l != lit_Error) {
                nStrengthened++;   
                if (occurence->size() > 1) {
                    Clause* strengthened = clause_db.strengthenClause(occurence, ~l);
                    occurences.add(strengthened);
                    return subsume(strengthened);
                } else {
                    return false;
                }
            }
        }
    }

    return true;
}

}
#endif