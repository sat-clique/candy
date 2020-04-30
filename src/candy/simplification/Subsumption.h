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

#include "candy/simplification/SubsumptionClause.h"
#include "candy/simplification/SubsumptionClauseDatabase.h"

#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/systems/propagate/Propagate.h"
#include "candy/mtl/Stamp.h"
#include "candy/frontend/CLIOptions.h"

namespace Candy {

class Subsumption { 
private:
    SubsumptionClauseDatabase& database;

    bool subsume(SubsumptionClause* clause);
    bool unit_resolution(SubsumptionClause* clause);

public:
    unsigned int nDuplicates;
    unsigned int nSubsumed;
    unsigned int nStrengthened;      

    Subsumption(SubsumptionClauseDatabase& database_)
     : database(database_), nDuplicates(0), nSubsumed(0), nStrengthened(0)
    { }

    bool subsume() {
        nDuplicates = nSubsumed = nStrengthened = 0; 
                
        for (const SubsumptionClause* clause : database) {
            if (!clause->is_deleted()) {
                // std::cout << "c Subsumption with clause " << *clause->get_clause() << "\r";
                if (clause->size() == 1) {
                    if (!unit_resolution((SubsumptionClause*)clause)) {
                        return false;
                    }
                }
                else {
                    if (!subsume((SubsumptionClause*)clause)) {
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

bool Subsumption::unit_resolution(SubsumptionClause* clause) {
    assert(clause->size() == 1);
    Lit unit = clause->get_clause()->first();
    const std::vector<SubsumptionClause*> occurences = database.copyOccurences(unit.var());
    for (SubsumptionClause* occurence : occurences) {
        if (occurence != clause && !occurence->is_deleted()) {
            if (occurence->contains(unit)) {
                // std::cout << "c " << unit << " - unit resolution removing clause " << *occurence->get_clause() << std::endl;
                database.remove(occurence);
            } else {
                if (occurence->size() == 1) {
                    return false;
                }
                // std::cout << "c " << unit << " - unit resolution strengthening clause " << *occurence->get_clause() << std::endl;
                database.strengthen(occurence, ~unit);
            }
        }
    }
    return true;
}

bool Subsumption::subsume(SubsumptionClause* subsumption_clause) {
    SubsumptionClause* clause = subsumption_clause;

    // Find best variable to scan:
    Lit best = *std::min_element(clause->begin(), clause->end(), [this] (Lit l1, Lit l2) {
        return database.numOccurences(l1.var()) < database.numOccurences(l2.var());
    });

    // Search all candidates:
    const std::vector<SubsumptionClause*> occurences = database.copyOccurences(best.var());
    for (const SubsumptionClause* occurence : occurences) {
        if (occurence != clause && !occurence->is_deleted()) {
            Lit l = clause->subsumes(occurence);

            if (l == lit_Undef) {
                if (clause->size() == occurence->size() && (clause->lbd() < occurence->lbd() || clause->lbd() == occurence->lbd() && clause->get_clause() < occurence->get_clause())) { 
                    // make sure each thread would deterministically delete the "same" duplicate
                    nDuplicates++;
                    // std::cout << "c Removing duplicate clause " << *occurence->get_clause() << std::endl;
                    database.remove((SubsumptionClause*)occurence);
                }
                else {
                    nSubsumed++;
                    if (occurence->get_clause()->isPersistent() && clause->get_clause()->isLearnt()) {
                        // recreate subsuming clause persistent
                        clause = database.create(clause->begin(), clause->end());
                        database.remove(subsumption_clause);
                    }
                    // std::cout << "c Removing subsumed clause " << *occurence->get_clause() << "(Subsumed by " << *clause->get_clause() << ")" << std::endl;
                    database.remove((SubsumptionClause*)occurence);
                }
            }
            else if (l != lit_Error) {
                nStrengthened++;   
                if (occurence->size() > 1) {
                    // std::cout << "c Creating self-subsuming resolvent of clauses " << *occurence->get_clause() << " and " << *clause->get_clause() << " using literal " << ~l << std::endl;
                    database.strengthen((SubsumptionClause*)occurence, ~l);
                    return subsume((SubsumptionClause*)occurence);
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