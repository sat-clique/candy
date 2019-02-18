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

#include <unordered_map>
#include <vector>

#include "candy/core/clauses/Clause.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/core/propagate/Propagate.h"
#include "candy/mtl/Stamp.h"
#include "candy/frontend/CLIOptions.h"

namespace Candy {

template <class TPropagate> 
class Subsumption { 
private:
    ClauseDatabase& clause_db;
    Trail& trail;
    TPropagate& propagator;

    const uint16_t subsumption_lim;   // Do not check if subsumption against a clause larger than this. 0 means no limit.

public:         
    Subsumption(ClauseDatabase& clause_db_, Trail& trail_, TPropagate& propagator_) : 
        clause_db(clause_db_),
        trail(trail_),
        propagator(propagator_),
        subsumption_lim(SubsumptionOptions::opt_subsumption_lim),
        queue(),
        abstractions(),
        nSubsumed(0),
        nStrengthened(0)
    {}

    std::vector<const Clause*> queue;
    std::unordered_map<const Clause*, uint64_t> abstractions;

    unsigned int nSubsumed;
    unsigned int nStrengthened;

    void attach(const Clause* clause) {
        if (abstractions.count(clause) == 0) {
            uint64_t abstraction = 0;
            for (Lit lit : *clause) {
                abstraction |= 1ull << (var(lit) % 64);
            }
            abstractions[clause] = abstraction;
            if (subsumption_lim == 0 || clause->size() < subsumption_lim) {
                queue.push_back(clause);
            }
        }
    }

    void clear() {
        queue.clear();
        abstractions.clear();
    }

    bool subsume();
    
}; 

template <class TPropagate> 
bool Subsumption<TPropagate>::subsume() {
    assert(trail.decisionLevel() == 0);
    
    nSubsumed = 0;
    nStrengthened = 0;
    
    for (const Clause* clause : clause_db) {
        attach(clause);
    }

    sort(queue.begin(), queue.end(), [](const Clause* c1, const Clause* c2) { 
        return c1->size() > c2->size() || (c1->size() == c2->size() && c1->getLBD() > c2->getLBD()); 
    });
    
    while (queue.size() > 0) {
        const Clause* clause = queue.back();
        queue.pop_back();

        if (clause->isDeleted()) continue;
        
        // Find best variable to scan:
        Var best = var(*std::min_element(clause->begin(), clause->end(), [this] (Lit l1, Lit l2) {
            return clause_db.numOccurences(var(l1)) < clause_db.numOccurences(var(l2));
        }));

        // Search all candidates:
        const uint64_t clause_abstraction = abstractions[clause];
        const std::vector<Clause*> occurences = clause_db.copyOccurences(best);
        for (const Clause* occurence : occurences) {
            if (occurence != clause && !occurence->isDeleted() && ((clause_abstraction & ~abstractions[occurence]) == 0)) {
                Lit l = clause->subsumes(*occurence);

                if (l == lit_Undef) { // remove:
                    if (clause->size() == occurence->size()) {
                        const Clause* to_delete = occurence;
                        if (clause->getLBD() > occurence->getLBD()) {
                            to_delete = clause;
                        }
                        else if (clause->getLBD() == occurence->getLBD()) {
                            if (clause > occurence) {
                                to_delete = clause;
                            }
                        }
                        clause_db.removeClause((Clause*)to_delete);
                        abstractions.erase(clause);
                        abstractions.erase(occurence);
                        break; 
                    }
                    else {
                        if (clause->isLearnt() && !occurence->isLearnt()) {
                            // in case of inprocessing: recreate persistent
                            Clause* persistent = clause_db.persistClause((Clause*)clause);
                            abstractions[persistent]=clause_abstraction;
                            abstractions.erase(clause);
                        }
                        nSubsumed++;
                        clause_db.removeClause((Clause*)occurence);
                        abstractions.erase(occurence);
                    }
                }
                else if (l != lit_Error) { // strengthen:
                    nStrengthened++;
                    
                    Clause* new_clause = clause_db.strengthenClause((Clause*)occurence, ~l);
                    abstractions.erase(occurence);
                    if (new_clause->size() == 0) {
                        return false;
                    }
                    else {
                        attach(new_clause);
                        if (new_clause->size() == 1) {
                            if (!trail.fact(new_clause->first()) || propagator.propagate() != nullptr) {
                                return false; 
                            }
                        } 
                    }
                }
            }
        }
    }

    propagator.clear();
    for (Clause* clause : clause_db) {
        if (clause->size() > 2) {
            propagator.attachClause(clause);
        }
    }
    clear();

    return true;
}

}