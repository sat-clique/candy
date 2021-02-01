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
    Trail& trail;

    void subsume(OccurenceList& occurences, Clause* clause);

public:
    unsigned int nDuplicates;
    unsigned int nSubsumed;
    unsigned int nStrengthened;

    unsigned int verbosity;

    Subsumption(ClauseDatabase& clause_db_, Trail& trail_) : 
        clause_db(clause_db_), trail(trail_), 
        nDuplicates(0), nSubsumed(0), nStrengthened(0), verbosity(SolverOptions::verb)
    { }

    void init() { }

    void subsume(OccurenceList& occurences) {
        nDuplicates = nSubsumed = nStrengthened = 0; 
        
        for (unsigned int i = 0; i < clause_db.size() && !clause_db.hasEmptyClause(); i++) {
            Clause* clause = clause_db[i];
            if (!clause->isDeleted()) {
                subsume(occurences, clause);
            }
        }
        
        if (verbosity > 1) {
            std::cout << "c Removed " << nDuplicates << " Duplicate Clauses" << std::endl;
            std::cout << "c Subsumption subsumed " << nSubsumed << " and strengthened " << nStrengthened << " clauses" << std::endl;
        }
    }

    unsigned int nTouched() {
        return nSubsumed + nStrengthened; 
    }
    
}; 

void Subsumption::subsume(OccurenceList& occurences, Clause* clause) {
    // Find best variable to scan:
    Lit best = *std::min_element(clause->begin(), clause->end(), [&occurences] (Lit l1, Lit l2) {
        return occurences.count(l1.var()) < occurences.count(l2.var());
    });
    for (unsigned int i = 0; i < occurences[best.var()].size() && !clause_db.hasEmptyClause(); i++) {
        Clause* occurence = occurences[best.var()][i];
        if (occurence != clause && !occurence->isDeleted()) {
            Lit l = clause->subsumes(occurence);

            if (l == lit_Undef) { 
                if (clause->size() == occurence->size()) {
                    nDuplicates++;
                } else {
                    nSubsumed++;
                }
                if (occurence->isPersistent()) clause->setPersistent();
                if (verbosity > 2) std::cout << *clause << " subsumes " << *occurence << std::endl;
                clause_db.removeClause(occurence);
            }
            else if (l != lit_Error) {
                nStrengthened++;   
                if (occurence->size() > 1) {
                    Clause* strengthened = clause_db.strengthenClause(occurence, ~l);
                    occurences.add(strengthened);
                    if (verbosity > 2) std::cout << *clause << " strengthens " << *occurence << std::endl;
                    subsume(occurences, strengthened);
                } 
                else {
                    clause_db.emptyClause();
                }
            }
        }
    }
}

}
#endif