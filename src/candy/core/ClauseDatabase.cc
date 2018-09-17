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
    allocator(), clauses(), learnts(), persist() {

}

ClauseDatabase::~ClauseDatabase() {
    for (Clause* c : clauses) {
        allocator.deallocate(c);
    }
    for (Clause* c : learnts) {
        allocator.deallocate(c);
    }
    for (Clause* c : persist) {
        allocator.deallocate(c);
    }
}

void ClauseDatabase::reduce() {
    Statistics::getInstance().solverReduceDBInc();
    std::sort(learnts.begin(), learnts.end(), reduceDB_lt());
    
    size_t index = (learnts.size() + persist.size()) / 2;
    if (index >= learnts.size() || learnts[index]->getLBD() <= 3) {
        return; // We have a lot of "good" clauses, it is difficult to compare them, keep more
    }
    
    // Delete clauses from the first half which are not locked. (Binary clauses are kept separately and are not touched here)
    // Keep clauses which seem to be useful (i.e. their lbd was reduce during this sequence => frozen)
    removed.clear();
    size_t limit = std::min(learnts.size(), index);
    for (size_t i = 0; i < limit && learnts[i]->getLBD() > persistentLBD; i++) {
        Clause* c = learnts[i];
        if (c->isFrozen()) {
            c->setFrozen(false); // reset flag
        }
        else if (trail.reason(var(c->first())) != c) {
            removed.push_back(c);
        }
    }

    Statistics::getInstance().solverRemovedClausesInc(removed.size());
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
    for (auto container : { clauses, learnts, persist }) {
        for (Clause* clause : container) {
            clause->activity() *= 1e-20f;
        }
    }
    cla_inc *= 1e-20;
}

}