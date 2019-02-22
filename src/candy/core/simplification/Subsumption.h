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

#include "candy/core/clauses/SubsumptionClause.h"
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

    std::vector<SubsumptionClause> list;

    void initialize();
    void unique();

public:   
    unsigned int nDuplicates;
    unsigned int nSubsumed;
    unsigned int nStrengthened;      

    Subsumption(ClauseDatabase& clause_db_, Trail& trail_, TPropagate& propagator_) : 
        clause_db(clause_db_), trail(trail_), propagator(propagator_),
        list(),
        nDuplicates(0), nSubsumed(0), nStrengthened(0)
    { }

    bool subsume();
    
}; 

template <class TPropagate> 
void Subsumption<TPropagate>::initialize() {
    list.clear();  

    nDuplicates = 0;
    nSubsumed = 0;
    nStrengthened = 0;  

    for (const Clause* clause : clause_db) {
        if (!clause->isDeleted()) {
            list.emplace_back(clause);
        }
    }

    sort(list.begin(), list.end(), [](const SubsumptionClause c1, const SubsumptionClause c2) { 
        return c1.size() < c2.size() || (c1.size() == c2.size() && c1.get_abstraction() < c2.get_abstraction()); 
    });
}

template <class TPropagate> 
void Subsumption<TPropagate>::unique() { // remove duplicates
    for (auto it1 = list.begin(); it1 != list.end(); it1++) {
        if (it1->get_clause()->isDeleted()) {
            continue;
        }
        for (auto it2 = it1+1; it2 != list.end() && it1->get_abstraction() == it2->get_abstraction(); it2++) {
            if (it2->get_clause()->isDeleted()) {
                continue;
            }
            if (it1->equals(*it2)) {
                nDuplicates++;
                if (it1->lbd() > it2->lbd() || (it1->lbd() == it2->lbd() && it1->get_clause() > it2->get_clause())) {
                    clause_db.removeClause(it1->get_clause());
                }
                else {
                    clause_db.removeClause(it2->get_clause());
                }
            }
        }
    }
}

template <class TPropagate> 
bool Subsumption<TPropagate>::subsume() {
    assert(trail.decisionLevel() == 0);

    initialize();
    unique();
    
    for (SubsumptionClause clause : list) {
        if (clause.get_clause()->isDeleted()) {
            continue;
        }
        
        // Find best variable to scan:
        Var best = var(*std::min_element(clause.begin(), clause.end(), [this] (Lit l1, Lit l2) {
            return clause_db.numOccurences(var(l1)) < clause_db.numOccurences(var(l2));
        }));

        // Search all candidates:
        const std::vector<Clause*> occurences = clause_db.copyOccurences(best);
        for (const Clause* occurence : occurences) {
            if (occurence == clause.get_clause() || occurence->isDeleted()) {
                continue;
            }

            Lit l = clause.subsumes(occurence);

            if (l == lit_Error) {
                continue;
            }

            if (l == lit_Undef) { // remove:
                nSubsumed++;
                if (clause.get_clause()->isLearnt() && !occurence->isLearnt()) {
                    clause_db.persistClause(clause.get_clause());
                }
                clause_db.removeClause((Clause*)occurence);
            }
            else { // strengthen:
                nStrengthened++;                
                Clause* new_clause = clause_db.strengthenClause((Clause*)occurence, ~l);
                if (new_clause->size() == 0) {
                    return false;
                }
                else if (new_clause->size() == 1) {
                    if (!trail.fact(new_clause->first()) || propagator.propagate() != nullptr) {
                        return false; 
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
    
    return true;
}

}