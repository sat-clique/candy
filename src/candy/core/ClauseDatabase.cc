#include "candy/core/ClauseDatabase.h"

#include "candy/utils/Options.h"

namespace Candy {

namespace ClauseDatabaseOptions {
    const char* _cat = "ClauseDatabase";

    Glucose::IntOption opt_persistent_lbd(_cat, "persistentLBD", "Minimum LBD value for learnt clauses to be kept persistent", 3, Glucose::IntRange(0, INT16_MAX));
    Glucose::IntOption opt_lb_lbd_frozen_clause(_cat, "minLBDFrozenClause", "Protect clauses if their LBD decrease and is lower than (for one turn)", 30, Glucose::IntRange(0, INT16_MAX));
    Glucose::DoubleOption opt_clause_decay(_cat, "cla-decay", "The clause activity decay factor", 0.999, Glucose::DoubleRange(0, false, 1, false));
}

ClauseDatabase::ClauseDatabase(Trail& trail_) : 
    trail(trail_), 
    persistentLBD(ClauseDatabaseOptions::opt_persistent_lbd),
    lbLBDFrozenClause(ClauseDatabaseOptions::opt_lb_lbd_frozen_clause),
    cla_inc(1), clause_decay(ClauseDatabaseOptions::opt_clause_decay),
    track_literal_occurrence(false),
    variableOccurrences(ClauseDeleted()),
    allocator(), 
    clauses() {

}

ClauseDatabase::~ClauseDatabase() {
}

void ClauseDatabase::initOccurrenceTracking(size_t nVars) {
    variableOccurrences.init(nVars);
    for (Clause* clause : clauses) {
        for (Lit lit : *clause) {
            variableOccurrences[var(lit)].push_back(clause);
        }
    }
    track_literal_occurrence = true;
}

void ClauseDatabase::stopOccurrenceTracking() {
    variableOccurrences.clear();
    track_literal_occurrence = false;
}

void ClauseDatabase::reduce() {
    Statistics::getInstance().solverReduceDBInc();

    std::vector<Clause*> learnts;
    copy_if(clauses.begin(), clauses.end(), std::back_inserter(learnts), [](Clause* clause) { return clause->isLearnt() && clause->size() > 2; });
    std::sort(learnts.begin(), learnts.end(), reduceDB_lt());

    learnts.erase(learnts.begin() + (learnts.size() / 2), learnts.end());    
    if (learnts.size() == 0 || learnts.back()->getLBD() <= persistentLBD) {
        return; // We have a lot of "good" clauses, it is difficult to compare them, keep more
    }
    
    // Delete clauses from the first half which are not locked. (Binary clauses are kept separately and are not touched here)
    // Keep clauses which seem to be useful (i.e. their lbd was reduce during this sequence => frozen)
    size_t count = 0;
    for (Clause* c : learnts) {
        if (c->isFrozen()) {
            c->setFrozen(false); // reset flag
        }
        else if (trail.reason(var(c->first())) != c) {
            ++count;
            removeClause(c);
        }
    }

    Statistics::getInstance().solverRemovedClausesInc(count);
}

/**
 * Make sure all references are updated after all clauses reside in a new adress space
 */
void ClauseDatabase::defrag() {
    vector<Clause*> reallocated = allocator.defrag(clauses);
    clauses.swap(reallocated);
}

size_t ClauseDatabase::nLearnts() const {
    return count_if(clauses.begin(), clauses.end(), [](Clause* clause) { return clause->isLearnt(); });
}

void ClauseDatabase::updateClauseActivitiesAndLBD(vector<Clause*>& involved_clauses, unsigned int learnt_lbd) {
    for (Clause* clause : involved_clauses) {
        claBumpActivity(*clause);

		// DYNAMIC NBLEVEL trick (see competition'09 Glucose companion paper)
		if (clause->isLearnt() && clause->getLBD() > persistentLBD) {
			uint_fast16_t nblevels = trail.computeLBD(clause->begin(), clause->end());
			if (nblevels + 1 < clause->getLBD()) { // improve the LBD
				if (clause->getLBD() <= lbLBDFrozenClause) {
					// seems to be interesting : keep it for the next round
					clause->setFrozen(true);
				}
				clause->setLBD(nblevels);
			}
		}
    }
}

void ClauseDatabase::claRescaleActivity() {
    for (Clause* clause : clauses) {
        clause->activity() *= 1e-20f;
    }
    cla_inc *= 1e-20;
}

}