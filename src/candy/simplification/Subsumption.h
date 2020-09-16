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
#include "candy/systems/propagate/Propagate.h"
#include "candy/mtl/Stamp.h"
#include "candy/utils/CLIOptions.h"

namespace Candy {

class Subsumption { 
private:
    OccurenceList& database;

    bool subsume(Clause* clause);
    bool unit_resolution(Clause* clause);

public:
    unsigned int nDuplicates;
    unsigned int nSubsumed;
    unsigned int nStrengthened;      

    Subsumption(OccurenceList& database_)
     : database(database_), nDuplicates(0), nSubsumed(0), nStrengthened(0)
    { }

    bool subsume() {
        nDuplicates = nSubsumed = nStrengthened = 0; 
                
        for (const Clause* clause : database) {
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
    const std::vector<Clause*> occurences = database.copyOccurences(unit.var());
    for (Clause* occurence : occurences) {
        if (occurence != clause && !occurence->isDeleted()) {
            if (occurence->contains(unit)) {
                // std::cout << "c " << unit << " - unit resolution removing clause " << *occurence << std::endl;
                database.remove(occurence);
            } else {
                if (occurence->size() == 1) {
                    return false;
                }
                // std::cout << "c " << unit << " - unit resolution strengthening clause " << *occurence << std::endl;
                database.strengthen(occurence, ~unit);
            }
        }
    }
    return true;
}

bool Subsumption::subsume(Clause* subsumption_clause) {
    Clause* clause = subsumption_clause;

    // Find best variable to scan:
    Lit best = *std::min_element(clause->begin(), clause->end(), [this] (Lit l1, Lit l2) {
        return database.numOccurences(l1.var()) < database.numOccurences(l2.var());
    });

    // Search all candidates:
    const std::vector<Clause*> occurences = database.copyOccurences(best.var());
    for (const Clause* occurence : occurences) {
        if (occurence != clause && !occurence->isDeleted()) {
            Lit l = clause->subsumes(occurence);

            if (l == lit_Undef) { 
                // make sure each thread would deterministically delete the "same" duplicate:
                if (clause->size() == occurence->size() && (clause->getLBD() < occurence->getLBD() || clause->getLBD() == occurence->getLBD() && clause < occurence)) {
                    nDuplicates++;
                    // std::cout << "c Removing duplicate clause " << *occurence << std::endl;
                    database.remove((Clause*)occurence);
                }
                else {
                    nSubsumed++;
                    if (occurence->isPersistent() && clause->isLearnt()) {
                        // recreate subsuming clause persistent
                        clause = database.create(clause->begin(), clause->end());
                        database.remove(subsumption_clause);
                    }
                    // std::cout << "c Removing subsumed clause " << *occurence << "(Subsumed by " << *clause << ")" << std::endl;
                    database.remove((Clause*)occurence);
                }
            }
            else if (l != lit_Error) {
                nStrengthened++;   
                if (occurence->size() > 1) {
                    // std::cout << "c Creating self-subsuming resolvent of clauses " << *occurence << " and " << *clause << " using literal " << ~l << std::endl;
                    database.strengthen((Clause*)occurence, ~l);
                    return subsume((Clause*)occurence);
                }
                else {
                    return false;
                }
            }
        }
    }

    return true;
}

}
#endif