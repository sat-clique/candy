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

#include <vector>
#include <stack>
#include <string>
#include <iostream>
#include <sstream>
#include <set>

#include "candy/core/clauses/Clause.h"
#include "candy/core/clauses/ClauseAllocator.h"
#include "candy/core/clauses/Certificate.h"
#include "candy/core/clauses/BinaryClauses.h"
#include "candy/core/clauses/Equivalences.h"
#include "candy/core/Trail.h"
#include "candy/core/CNFProblem.h"
#include "candy/utils/CLIOptions.h"
#include "candy/mtl/State.h"

#ifndef CANDY_CLAUSE_DATABASE
#define CANDY_CLAUSE_DATABASE

namespace Candy {

struct AnalysisResult {
	AnalysisResult() : 
		nConflicts(0), learnt_clause(), involved_clauses(), lbd(0), backtrack_level(0)
	{ }

	uint64_t nConflicts;
	std::vector<Lit> learnt_clause;
	std::vector<Reason> involved_clauses;
	unsigned int lbd;
    unsigned int backtrack_level;

    void setLearntClause(std::vector<Lit>& learnt_clause_, std::vector<Reason>& involved_clauses_, unsigned int lbd_, unsigned int backtrack_level_) {
        assert(lbd_ <= learnt_clause_.size());
        nConflicts++;
        learnt_clause.swap(learnt_clause_);
        involved_clauses.swap(involved_clauses_); 
        lbd = lbd_;
        backtrack_level = backtrack_level_;
    }

};

class ClauseDatabase {
private:
    ClauseAllocator allocator;
 
    unsigned int variables;
    std::vector<Clause*> clauses; // Working set of problem clauses
    bool emptyClause_;

    Certificate certificate;

public:
    std::vector<double> occurrence;

    std::vector<Lit> unaries;
    BinaryClauses binaries;

    /* analysis result is stored here */
	AnalysisResult result;
    Equivalences equiv;

    ClauseDatabase(CNFProblem& problem) : 
        allocator(), variables(problem.nVars()), clauses(), emptyClause_(false), 
        certificate(SolverOptions::opt_certified_file), 
        occurrence(2 * problem.nVars(), 0.0),
        unaries(), binaries(problem.nVars()), result(), equiv(binaries)
    { 
        for (Cl* import : problem) {
            createClause(import->begin(), import->end(), 0, true);
        }
    }

    ~ClauseDatabase() { }

    unsigned int nVars() const {
        return variables;
    }

    ClauseAllocator* createGlobalClauseAllocator() {
        return allocator.create_global_allocator();
    }

    void setGlobalClauseAllocator(ClauseAllocator* global_allocator) {
        allocator.set_global_allocator(global_allocator);
        this->clauses = allocator.collect();
        unaries.clear();
        binaries.clear();
        for (Clause* clause : clauses) {
            if (clause->size() == 1) {
                unaries.push_back(clause->first());
            } else if (clause->size() == 2) {
                binaries.add(clause);
            }
        }
    }

    void reorganize() {
        allocator.synchronize(); // inactive if no global-allocator
        allocator.reorganize(); // defrag.
        clauses = allocator.collect();
        if (SolverOptions::opt_sort_variables == 4) for (Clause* c : clauses) c->sort(occurrence, true);
        else if (SolverOptions::opt_sort_variables == 5) for (Clause* c : clauses) c->sort(occurrence, false);
        else if (SolverOptions::opt_sort_variables == 8) for (Clause* c : clauses) c->sort2(occurrence, true);
        else if (SolverOptions::opt_sort_variables == 9) for (Clause* c : clauses) c->sort2(occurrence, false);

        if (SolverOptions::opt_sort_clauses == 1) {
            std::sort(clauses.begin(), clauses.end(), [](Clause* c1, Clause* c2) { return c1->size() < c2->size(); } );
        }
        else if (SolverOptions::opt_sort_clauses == 2) {
            std::sort(clauses.begin(), clauses.end(), [](Clause* c1, Clause* c2) { return c1->getLBD() < c2->getLBD(); } );
        }
        else if (SolverOptions::opt_sort_clauses == 3) {
            std::sort(clauses.begin(), clauses.end(), [](Clause* c1, Clause* c2) { return c1->size() == c2->size() ? c1->getLBD() < c2->getLBD() : c1->size() < c2->size(); } );
        }
        else if (SolverOptions::opt_sort_clauses == 4) {
            std::sort(clauses.begin(), clauses.end(), [](Clause* c1, Clause* c2) { return c1->size() > c2->size(); } );
        }
        else if (SolverOptions::opt_sort_clauses == 5) {
            std::sort(clauses.begin(), clauses.end(), [](Clause* c1, Clause* c2) { return c1->getLBD() > c2->getLBD(); } );
        }
        else if (SolverOptions::opt_sort_clauses == 6) {
            std::sort(clauses.begin(), clauses.end(), [](Clause* c1, Clause* c2) { return c1->size() == c2->size() ? c1->getLBD() > c2->getLBD() : c1->size() > c2->size(); } );
        }
        unaries.clear();
        binaries.clear();
        for (Clause* clause : clauses) {
            if (clause->size() == 1) {
                unaries.push_back(clause->first());
            } else if (clause->size() == 2) {
                binaries.add(clause);
            }
        }
    }

    typedef std::vector<Clause*>::const_iterator const_iterator;

    inline const_iterator begin() const {
        return clauses.begin();
    }

    inline const_iterator end() const {
        return clauses.end();
    }

    inline unsigned int size() const {
        return clauses.size();
    }

    inline Clause* operator [](int i) const {
        return clauses[i];
    }

    bool hasEmptyClause() const {
        return emptyClause_;
    }

    void emptyClause() {
        emptyClause_ = true;
        certificate.proof();
    }

    template<typename Iterator>
    inline Clause* createClause(Iterator begin, Iterator end, unsigned int lbd = 0, bool lemma = false) {
        unsigned int length = std::distance(begin, end);
        unsigned int weight = length < 3 ? 0 : lbd;

        Clause* clause = new (allocator.allocate(length, weight)) Clause(begin, end, weight);
        clauses.push_back(clause);

        if (!lemma) certificate.added(clause->begin(), clause->end());

        if (clause->size() == 0) {
            emptyClause_ = true;
        }
        else if (clause->size() == 1) {
            unaries.push_back(clause->first());
        }
        else if (clause->size() == 2) {
            binaries.add(clause);
        }

        for (Lit lit : *clause) {
            occurrence[lit] += 1.0 / pow(2, clause->size());
        }
        
        return clause;
    }

    inline void removeClause(Clause* clause) {
        allocator.deallocate(clause);
        certificate.removed(clause->begin(), clause->end());

        for (Lit lit : *clause) {
            occurrence[lit] -= 1.0 / pow(2, clause->size());
        }
    }

    Clause* strengthenClause(Clause* clause, Lit lit) {
        assert(clause->size() > 1);
        std::vector<Lit> literals;
        for (Lit literal : *clause) if (literal != lit) literals.push_back(literal);
        Clause* new_clause = createClause(literals.begin(), literals.end(), std::min((uint16_t)clause->getLBD(), (uint16_t)literals.size()));
        removeClause(clause);
        return new_clause;
    }

};

}

#endif