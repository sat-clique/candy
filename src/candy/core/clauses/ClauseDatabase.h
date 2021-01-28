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
	std::vector<Clause*> involved_clauses;
	unsigned int lbd;
    unsigned int backtrack_level;

    void setLearntClause(std::vector<Lit>& learnt_clause_, std::vector<Clause*>& involved_clauses_, unsigned int lbd_, unsigned int backtrack_level_) {
        assert(lbd_ <= learnt_clause_.size());
        nConflicts++;
        learnt_clause.swap(learnt_clause_);
        involved_clauses.swap(involved_clauses_); 
        lbd = lbd_;
        backtrack_level = backtrack_level_;
    }

};

struct BinaryWatcher {
    Clause* clause;
    Lit other;

    BinaryWatcher(Clause* _clause, Lit _other)
     : clause(_clause), other(_other) { }

};

class ClauseDatabase {
private:
    ClauseAllocator allocator;
 
    unsigned int variables;
    std::vector<Clause*> clauses; // Working set of problem clauses
    bool emptyClause_;

    Certificate certificate;

public:
    std::vector<std::vector<BinaryWatcher>> binary_watchers;
    std::vector<Lit> equiv;
    
    /* analysis result is stored here */
	AnalysisResult result;

    ClauseDatabase() : 
        allocator(), variables(0), clauses(), emptyClause_(false), 
        certificate(SolverOptions::opt_certified_file),
        binary_watchers(), equiv(), result()
    { }

    ~ClauseDatabase() { }

	void greedy_cycle(Lit root) {
        // std::stringbuf buf;
        // std::ostream os(&buf);
        static Stamp<uint8_t> block(2*nVars());
        if (block[root]) return;
        block.set(root);
		static State<uint8_t, 3> mark(2*nVars());
        mark.clear();
		std::vector<Lit> parent(2*nVars(), lit_Undef);
		std::stack<Lit> stack;
		stack.push(root);
		mark.set(root, 2);
		while (!stack.empty()) {
			Lit lit = stack.top(); stack.pop();
			for (BinaryWatcher& watcher : binary_watchers[lit]) {
                Lit impl = watcher.other;
				if (mark[impl] == 0) {
					parent[impl] = lit;
					mark.set(impl, 1);
					stack.push(impl);
                    // os << *child.clause << std::endl;
				}
				else if (mark[impl] == 2) {
                    // std::cout << buf.str();
                    // std::cout << "found cycle on root " << root << " with clause " << *child.clause << ": ";
                    // for (Lit iter = parent[lit.var()]; iter != lit_Undef && mark[iter] != 2; iter = parent[iter.var()]) std::cout << " " << iter << " <-> " << lit;
                    // std::cout << std::endl;
					parent[impl] = lit;
                    Lit iter = impl;
                    while (mark[parent[iter]] != 2) {
                        block.set(iter);
                        set_eq(parent[iter], impl);
						mark.set(iter, 2);
                        iter = parent[iter];
                    }
				}
			}
		}
	}

    void set_eq(Lit lit1, Lit lit2) {
        Lit rep1 = get_eq(lit1);
        Lit rep2 = get_eq(lit2);
        equiv[rep1.var()] = rep1.sign() ? ~rep2 : rep2;
    }

    Lit get_eq(Lit lit) {
        return lit.sign() ? ~get_eq(lit.var()) : get_eq(lit.var());
    }

    Lit get_eq(Var v) {
        Lit rep = equiv[v];
        if (rep.var() != v) rep = rep.sign() ? ~get_eq(rep.var()) : get_eq(rep.var());
        equiv[v] = rep;
        return rep;
    }

    void clear() {
        binary_watchers.clear();
        equiv.clear();
        allocator.clear();
        clauses.clear();
        variables = 0;
        emptyClause_ = false;
        result.nConflicts = 0;
    }

    unsigned int nVars() {
        return variables;
    }

    void init(const CNFProblem& problem, ClauseAllocator* global_allocator = nullptr, bool lemma = true) {
        if (variables < problem.nVars()) {
            binary_watchers.resize(problem.nVars()*2+2);
            equiv.resize(problem.nVars()+1);
            for (unsigned int i = variables; i < problem.nVars(); i++) {
                equiv[i] = Lit(i);
            }
            variables = problem.nVars();
        }
        for (Cl* import : problem) {
            createClause(import->begin(), import->end(), lemma ? 0 : import->size(), lemma);
        }
        if (global_allocator != nullptr) setGlobalClauseAllocator(global_allocator);
    }

    void reinitBinaryWatchers() {
        binary_watchers.clear();
        binary_watchers.resize(variables*2+2);
        for (Clause* clause : clauses) {
            if (clause->size() == 2) {
                binary_watchers[~clause->first()].emplace_back(clause, clause->second());
                binary_watchers[~clause->second()].emplace_back(clause, clause->first());
            }
        }
    }

    ClauseAllocator* createGlobalClauseAllocator() {
        return allocator.create_global_allocator();
    }

    void setGlobalClauseAllocator(ClauseAllocator* global_allocator) {
        allocator.set_global_allocator(global_allocator);
        this->clauses = allocator.collect();
        reinitBinaryWatchers();
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
        else if (clause->size() == 2) {
            binary_watchers[~clause->first()].emplace_back(clause, clause->second());
            binary_watchers[~clause->second()].emplace_back(clause, clause->first());
        }
        
        return clause;
    }

    inline void removeClause(Clause* clause) {
        allocator.deallocate(clause);

        certificate.removed(clause->begin(), clause->end());

        if (clause->size() == 2) {
            std::vector<BinaryWatcher>& list0 = binary_watchers[~clause->first()];
            std::vector<BinaryWatcher>& list1 = binary_watchers[~clause->second()];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [clause](BinaryWatcher w){ return w.clause == clause; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [clause](BinaryWatcher w){ return w.clause == clause; }), list1.end());
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

    std::vector<Clause*> getUnitClauses() { 
        return allocator.collect_unit_clauses();
    }

    /**
     * Make sure all references are updated after all clauses reside in a new adress space
     */
    void reorganize() {
        allocator.synchronize();
        allocator.reorganize();
        clauses = allocator.collect();
        reinitBinaryWatchers();
    }

};

}

#endif