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

#include "candy/core/DRATChecker.h"

#include "candy/utils/StreamBuffer.h"
#include "candy/utils/Exceptions.h"

#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <fstream>
#include <iostream>

namespace Candy {

DRATChecker::DRATChecker(CNFProblem& problem_)
 : problem(problem_), clause_db(), trail(), propagator(clause_db, trail), occurences(), num_deleted(0) 
{ 
    clause_db.init(problem);
    trail.init(clause_db.nVars());
    propagator.init();

    occurences.resize(2 * clause_db.nVars());
    for (Clause* clause : clause_db) {
        for (Lit lit : *clause) {
            occurences[lit].push_back(clause);
        }
        if (clause->size() == 1) {
            trail.fact(clause->first());
        }
    }
}

DRATChecker::~DRATChecker() { 
    clause_db.clear();
    trail.clear();
    propagator.clear();
}

bool DRATChecker::check_proof(const char* filename) {
    gzFile in = gzopen(filename, "rb");
    if (in == NULL) {
        throw ParserException("ERROR! Could not open file");
    }
    bool result = clause_db.hasEmptyClause() || check_proof(in);
    gzclose(in);
    return result;
}

long DRATChecker::proof_size(const char* filename) {
    struct stat stat_buf;
    int rc = stat(filename, &stat_buf);
    return (rc == 0) ? stat_buf.st_size : -1;
}

bool DRATChecker::check_proof(gzFile input_stream) {
    Cl lits;
    StreamBuffer in(input_stream);
    in.skipWhitespace();
    while (!in.eof()) {
        if (*in == 'c') {
            in.skipLine();
        }
        else if (*in == 'd') {
            ++in;
            lits.clear();
            for (int plit = in.readInteger(); plit != 0; plit = in.readInteger()) {
                lits.push_back(Lit(abs(plit)-1, plit < 0));
            }
            // std::cout << "c *** Checking Delete: " << lits << " *** " << std::endl;
            check_clause_remove(lits.begin(), lits.end());
        }
        else {
            lits.clear();
            for (int plit = in.readInteger(); plit != 0; plit = in.readInteger()) {
                lits.push_back(Lit(abs(plit)-1, plit < 0));
            }
            // std::cout << "c *** Checking Learned: " << lits << " *** " << std::endl;
            if (!check_clause_add(lits.begin(), lits.end())) {
                return false;
            }
        }
        in.skipWhitespace();
    }
    return clause_db.hasEmptyClause();
}

template <typename Iterator>
bool DRATChecker::check_asymm(Iterator begin, Iterator end, Lit pivot) {
    if (clause_db.hasEmptyClause()) return true;
    // check if clause is asymmetric tautology
    for (auto it = begin; it != end; it++) if (*it != pivot) {
        Lit lit = ~*it;
        if (trail.falsifies(lit)) {
            return true;
        }
        else if (!trail.satisfies(lit)) {
            trail.newDecisionLevel();
            trail.decide(lit);
        }
    }
    return propagator.propagate() != nullptr;
}

template <typename Iterator>
bool DRATChecker::check_clause_add(Iterator begin, Iterator end) {
    // check if clause is asymmetric tautology
    bool conflict = check_asymm(begin, end, lit_Undef);
        
    // check if clause is resolution asymmetric tautology
    unsigned int level = trail.decisionLevel();
    for (auto it = begin; !conflict && it != end; it++) {
        Lit pivot = *it;
        for (Clause* clause : occurences[~pivot]) if (!clause->isDeleted()) {
            conflict = check_asymm(clause->begin(), clause->end(), ~pivot);
            trail.backtrack(level);
            if (!conflict) break;
        }
    }
    trail.backtrack(0);

    if (conflict) {
        Clause* clause = clause_db.createClause(begin, end);
        if (clause->size() > 2) {
            propagator.attachClause(clause);
        }
        for (Lit lit : *clause) {
            occurences[lit].push_back(clause);
        }
        if (clause->size() == 1 && !trail.fact(clause->first())) {
            clause_db.emptyClause();
        }
        return true;
    }
    return false;
}

template <typename Iterator>
bool DRATChecker::check_clause_remove(Iterator begin, Iterator end) {
    size_t size = std::distance(begin, end);
    if (size > 1) {
        for (Clause* clause : occurences[*begin]) {
            if (size == clause->size() && std::all_of(begin+1, end, [clause](Lit lit) { return clause->contains(lit); })) {
                clause_db.removeClause(clause);
                if (clause->size() > 2) {
                    propagator.detachClause(clause);
                }
                if (++num_deleted * 10 > clause_db.size()) cleanup_deleted();
                return true;
            }
        }
    }
    return false;
}

void DRATChecker::cleanup_deleted() {
    clause_db.reorganize();
    propagator.reset();
    occurences.clear();
    occurences.resize(2 * clause_db.nVars());
    for (Clause* clause : clause_db) {
        for (Lit lit : *clause) {
            occurences[lit].push_back(clause);
        }
    }
}

}
