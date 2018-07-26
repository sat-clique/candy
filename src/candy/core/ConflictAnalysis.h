/*
 * ConflictAnalysis.h
 *
 *  Created on: 01.10.2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CONFLICTANALYSIS_H_
#define SRC_CANDY_CORE_CONFLICTANALYSIS_H_

#include "candy/core/Stamp.h"
#include "candy/core/SolverTypes.h"
#include "candy/core/Statistics.h"
#include "candy/core/Trail.h"
#include "candy/core/Propagate.h"
#include "candy/core/Clause.h"
#include "candy/utils/CheckedCast.h"
#include <vector>

namespace Candy {

class ConflictAnalysis {
private:
	/* some helper data-structures */
    Stamp<uint32_t> stamp;
    std::vector<Var> analyze_clear;
    std::vector<Var> analyze_stack;

    /* analysis result is stored here */
    std::vector<Lit> learnt_clause;
    std::vector<Clause*> involved_clauses;

    /* pointers to solver state */
    Trail* trail;
    Propagate* propagate;

    /* Constant for reducing clause */
    unsigned int lbSizeMinimizingClause;

    inline uint64_t abstractLevel(Var x) const {
        return 1ull << (trail->level(x) % 64);
    }

	// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
	// visiting literals at levels that cannot be removed later.
	bool litRedundant(Lit lit, uint64_t abstract_levels) {
		size_t top = analyze_clear.size();

	    analyze_stack.clear();
	    analyze_stack.push_back(var(lit));

	    while (analyze_stack.size() > 0) {
	        assert(trail->reason(analyze_stack.back()) != nullptr);

	        Clause* clause = trail->reason(analyze_stack.back());
	        analyze_stack.pop_back();

	        if (clause->size() == 2 && trail->value(clause->first()) == l_False) {
	            assert(trail->value(clause->second()) == l_True);
	            clause->swap(0, 1);
	        }

	        for (Lit imp : *clause) {
	            Var v = var(imp);
	            if (!stamp[v] && trail->level(v) > 0) {
	                if (trail->reason(v) != nullptr && (abstractLevel(v) & abstract_levels) != 0) {
	                    stamp.set(v);
	                    analyze_stack.push_back(v);
	                    analyze_clear.push_back(v);
	                } else {
	                	auto begin = analyze_clear.begin() + top;
	                	for_each(begin, analyze_clear.end(), [this](Var v) { stamp.unset(v); });
	                	analyze_clear.erase(begin, analyze_clear.end());
	                    return false;
	                }
	            }
	        }
	    }

	    return true;
	}

	/******************************************************************
	 * Minimisation with binary clauses of the asserting clause
	 ******************************************************************/
	void minimisationWithBinaryResolution() {
	    stamp.clear();

	    for (auto it = learnt_clause.begin()+1; it != learnt_clause.end(); it++) {
	        stamp.set(var(*it));
	    }

	    bool minimize = false;
	    for (Watcher w : propagate->getBinaryWatchers(~learnt_clause[0])) {
	        if (stamp[var(w.blocker)] && trail->value(w.blocker) == l_True) {
	            minimize = true;
	            stamp.unset(var(w.blocker));
	        }
	    }

	    if (minimize) {
	        auto end = std::remove_if(learnt_clause.begin()+1, learnt_clause.end(), [this] (Lit lit) { return !stamp[var(lit)]; } );
	        Statistics::getInstance().solverReducedClausesInc(std::distance(end, learnt_clause.end()));
	        learnt_clause.erase(end, learnt_clause.end());
	    }
	}

public:
	ConflictAnalysis(Trail* trail_, Propagate* propagate_, unsigned int lbSizeMinimizingClause_) :
		stamp(),
		analyze_clear(),
		analyze_stack(),
		learnt_clause(),
		involved_clauses(),
		trail(trail_),
		propagate(propagate_),
		lbSizeMinimizingClause(lbSizeMinimizingClause_)
	{

	}

	~ConflictAnalysis() {

	}

	std::vector<Lit>* getLearntClause() {
		return &learnt_clause;
	}

	std::vector<Clause*>* getInvolvedClauses() {
		return &involved_clauses;
	}

	void incSize() {
		stamp.incSize();
	}

	void incSize(size_t size) {
		stamp.incSize(size);
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
	void analyze(Clause* confl) {
	    int pathC = 0;
	    Lit asslit = lit_Undef;
	    stamp.clear();
	    learnt_clause.clear(),
	    involved_clauses.clear();

	    // Generate conflict clause:
	    learnt_clause.push_back(lit_Undef); // (leave room for the asserting literal)
	    Trail::const_reverse_iterator trail_iterator = trail->rbegin();
	    do {
	        assert(confl != nullptr); // (otherwise should be UIP)
	        involved_clauses.push_back(confl);

	        // Special case for binary clauses: The first one has to be SAT
	        if (asslit != lit_Undef && confl->size() == 2 && trail->value(confl->first()) == l_False) {
	            assert(trail->value(confl->second()) == l_True);
	            confl->swap(0, 1);
	        }

	        for (auto it = (asslit == lit_Undef) ? confl->begin() : confl->begin() + 1; it != confl->end(); it++) {
	            Var v = var(*it);
	            if (!stamp[v] && trail->level(v) != 0) {
	                stamp.set(v);
	                if (trail->level(v) >= (int)trail->decisionLevel()) {
	                    pathC++;
	                } else {
	                    learnt_clause.push_back(*it);
	                }
	            }
	        }

	        // Select next clause to look at:
	        while (!stamp[var(*trail_iterator)]) {
	            trail_iterator++;
	        }

	        asslit = *trail_iterator;
	        stamp.unset(var(*trail_iterator));
	        confl = trail->reason(var(*trail_iterator));
	        pathC--;
	    } while (pathC > 0);

	    learnt_clause[0] = ~asslit;

	    // Simplify conflict clause:
	    Statistics::getInstance().solverMaxLiteralsInc(checked_unsigned_cast<std::remove_reference<decltype(learnt_clause)>::type::size_type, unsigned int>(learnt_clause.size()));

	    // minimize clause
	    analyze_clear.clear();
	    uint64_t abstract_level = 0;
	    for (uint_fast16_t i = 1; i < learnt_clause.size(); i++) {
	        abstract_level |= abstractLevel(var(learnt_clause[i])); // (maintain an abstraction of levels involved in conflict)
	    }
	    auto end = remove_if(learnt_clause.begin()+1, learnt_clause.end(),
	                         [this, abstract_level] (Lit lit) {
	                             return trail->reason(var(lit)) != nullptr && litRedundant(lit, abstract_level);
	                         });
	    learnt_clause.erase(end, learnt_clause.end());

	    assert(learnt_clause[0] == ~asslit);

	    if (learnt_clause.size() <= lbSizeMinimizingClause) {
	        minimisationWithBinaryResolution();
	    }

	    Statistics::getInstance().solverTotLiteralsInc(checked_unsigned_cast<std::remove_reference<decltype(learnt_clause)>::type::size_type, unsigned int>(learnt_clause.size()));
	}

	/**************************************************************************************************
	 *
	 *  analyzeFinal : (p : Lit)  ->  [void]
	 *
	 *  Description:
	 *    Specialized analysis procedure to express the final conflict in terms of assumptions.
	 *    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
	 *    stores the result in 'out_conflict'.
	 |*************************************************************************************************/
	void analyzeFinal(Lit p) {
	    learnt_clause.clear();
	    learnt_clause.push_back(p);

	    if (trail->decisionLevel() == 0)
	        return;

	    stamp.clear();

	    stamp.set(var(p));

	    for (int i = trail->size() - 1; i >= (int)trail->trail_lim[0]; i--) {
	        Var x = var((*trail)[i]);
	        if (stamp[x]) {
	            if (trail->reason(x) == nullptr) {
	                assert(trail->level(x) > 0);
	                learnt_clause.push_back(~(*trail)[i]);
	            } else {
	                Clause* c = trail->reason(x);
	                for (Lit lit : *c) {
	                    if (trail->level(var(lit)) > 0) {
	                        stamp.set(var(lit));
	                    }
	                }
	            }
	            stamp.unset(x);
	        }
	    }
	    stamp.unset(var(p));
	}

};

}
#endif /* SRC_CANDY_CORE_CONFLICTANALYSIS_H_ */
