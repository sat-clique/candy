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

#ifndef SRC_CANDY_CORE_CONFLICTANALYSIS_H_
#define SRC_CANDY_CORE_CONFLICTANALYSIS_H_

#include "candy/mtl/Stamp.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/Trail.h"
#include "candy/core/clauses/Clause.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/utils/CheckedCast.h"
#include "candy/frontend/CLIOptions.h"
#include <vector>

namespace Candy {

class ConflictAnalysis {
private:
	std::vector<Lit> learnt_clause;
	std::vector<Clause*> involved_clauses;

	/* some helper data-structures */
    Stamp<uint32_t> stamp;
    std::vector<Var> analyze_clear;
    std::vector<Var> analyze_stack;
	std::vector<Lit> minimized;

    /* pointers to solver state */
	ClauseDatabase& clause_db; 
    Trail& trail;

    inline uint64_t abstractLevel(Var x) const {
        return 1ull << (trail.level(x) % 64);
    }

	// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
	// visiting literals at levels that cannot be removed later.
	bool litRedundant(Lit lit, uint64_t abstract_levels) {
		unsigned int clear_start = analyze_clear.size();

	    analyze_stack.clear();
	    analyze_stack.push_back(lit.var());

	    while (!analyze_stack.empty()) {
	        const Clause* clause = trail.reason(analyze_stack.back());
	        analyze_stack.pop_back();

	        for (Lit imp : *clause) {
	            Var v = imp.var();
	            if (!stamp[v] && trail.level(v) > 0) {
	                if (trail.reason(v) != nullptr && (abstractLevel(v) & abstract_levels) != 0) {
	                    stamp.set(v);
	                    analyze_stack.push_back(v);
	                    analyze_clear.push_back(v);
	                } else {
						for (unsigned int i = clear_start; i < analyze_clear.size(); ++i) {
							stamp.unset(analyze_clear[i]);
						}
						analyze_clear.resize(clear_start);
	                    return false;
	                }
	            }
	        }
	    }

	    return true;
	}

	void minimization() {
	    analyze_clear.clear();
		minimized.clear();

	    uint64_t abstract_level = 0;
	    for (unsigned int i = 1; i < learnt_clause.size(); ++i) {
	        abstract_level |= abstractLevel(learnt_clause[i].var()); // (maintain an abstraction of levels involved in conflict)
	    }
		
		for (Lit lit : learnt_clause) {
			if (minimized.size() == 0) { // keep asserted literal
				minimized.push_back(lit);
			}
			else if (trail.reason(lit.var()) == nullptr) {
				minimized.push_back(lit);
			}
			else if (!litRedundant(lit, abstract_level)) {
				minimized.push_back(lit);
			}
		}
		learnt_clause.swap(minimized);
	}

	/******************************************************************
	 * Minimisation with binary clauses of the asserting clause
	 ******************************************************************/
	void minimizationWithBinaryResolution() {
	    stamp.clear();

	    bool minimize = false;
	    for (BinaryWatcher w : clause_db.getBinaryWatchers(~learnt_clause[0])) {
	        if (trail.satisfies(w.other)) {
	            minimize = true;
	            stamp.set(w.other.var());
	        }
	    }

	    if (minimize) {
	        auto end = std::remove_if(learnt_clause.begin()+1, learnt_clause.end(), [this] (Lit lit) { return stamp[lit.var()]; } );
	        learnt_clause.erase(end, learnt_clause.end());
	    }
	}

	/**************************************************************************************************
	 *
	 *  analyze : (confl : Clause*) (out_learnt : vector<Lit>&) (out_btlevel : int&)  ->  [void]
	 *
	 *  Description:
	 *    Analyze conflict and produce a reason clause.
	 *
	 *    Pre-conditions:
	 *      - 'out_learnt' is assumed to be cleared.
	 *      - Current decision level must be greater than root level.
	 *
	 *    Post-conditions:
	 *      - 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
	 *      - If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
	 *        rest of literals. There may be others from the same level though.
	 *
	 ***************************************************************************************************/
	void analyze(const Clause* confl) {
	    learnt_clause.push_back(lit_Undef); // (leave room for the asserting literal)
	    stamp.clear();

	    Lit asserted_literal = lit_Undef;
	    auto trail_iterator = trail.rbegin();
	    for(int pathC = 0; pathC > 0 || asserted_literal == lit_Undef; pathC--) {
	        assert(confl != nullptr); // (otherwise should be UIP)
	        involved_clauses.push_back((Clause*)confl);

	        for (Lit lit : *confl) {
				assert((trail.value(lit) == l_True) == (lit == asserted_literal));
				Var v = lit.var();
				if (lit != asserted_literal && !stamp[v]) {
					unsigned int level = trail.level(v);
	                stamp.set(v);
	                if (level >= trail.decisionLevel()) {
	                    ++pathC;
	                } else if (level > 0) {
	                    learnt_clause.push_back(lit);
	                }
	            }
	        }

	        // Select next clause to look at:
	        while (!stamp[trail_iterator->var()]) {
	            ++trail_iterator;
	        }

	        asserted_literal = *trail_iterator;
	        stamp.unset(trail_iterator->var());
	        confl = trail.reason(trail_iterator->var());
	    }

	    learnt_clause[0] = ~asserted_literal;


	    // Minimize conflict clause:
		minimization();
	    minimizationWithBinaryResolution();
		
	    assert(learnt_clause[0] == ~asserted_literal);
	}

public:
	ConflictAnalysis(ClauseDatabase& _clause_db, Trail& _trail) :
		stamp(),
		analyze_clear(),
		analyze_stack(),
		clause_db(_clause_db),
		trail(_trail)
	{ }

	~ConflictAnalysis() { }

	void init(unsigned int nVars) {
		if (nVars >= stamp.size()) {
			stamp.grow(nVars);
		}
	}

	void handle_conflict(Clause* confl) {
		learnt_clause.clear();
		involved_clauses.clear();

		analyze(confl);

		unsigned int lbd = trail.computeLBD(learnt_clause.begin(), learnt_clause.end());
        
		unsigned int backtrack_level = 0;
		if (learnt_clause.size() > 1) {
			backtrack_level = trail.level(learnt_clause[1].var());
			for (unsigned int i = 2; i < learnt_clause.size(); ++i) {
				unsigned int level = trail.level(learnt_clause[i].var());
				if (level > backtrack_level) {
					backtrack_level = level;
					std::swap(learnt_clause[1], learnt_clause[i]);
				}
			}
		}

		clause_db.reestimateClauseWeights(trail, involved_clauses);
		clause_db.setLearntClause(learnt_clause, involved_clauses, lbd, backtrack_level); 
	}

	/**************************************************************************************************
	 *
	 *  analyzeFinal : (p : Lit)  ->  std::vector<Lit>
	 *
	 *  Specialized analysis procedure to express the final conflict in terms of assumptions.
	 *  Calculates and returns the set of assumptions that led to the assignment of 'p'.
	 * 
	 |*************************************************************************************************/
	std::vector<Lit> analyzeFinal(Lit p) { 
		std::vector<Lit> assumptions;
	    assumptions.push_back(p);

	    if (trail.decisionLevel() > 0) {
			stamp.clear();
			stamp.set(p.var());
			for (int i = trail.size() - 1; i >= (int)trail.trail_lim[0]; i--) {
				Var x = trail[i].var();
				if (stamp[x]) {
					if (trail.reason(x) == nullptr) {
						assert(trail.level(x) > 0);
						assumptions.push_back(~trail[i]);
					} else {
						const Clause* c = trail.reason(x);
						for (Lit lit : *c) {
							if (trail.level(lit.var()) > 0) {
								stamp.set(lit.var());
							}
						}
					}
					stamp.unset(x);
				}
			}
			stamp.unset(p.var());
		}

		return assumptions;
	}

};

}
#endif /* SRC_CANDY_CORE_CONFLICTANALYSIS_H_ */
