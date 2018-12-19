#include "candy/core/ClauseDatabase.h"

#include "candy/core/ClauseAllocator.h"

namespace Candy {

ClauseDatabase::ClauseDatabase() : 
    persistentLBD(ClauseDatabaseOptions::opt_persistent_lbd),
    reestimationReduceLBD(ClauseDatabaseOptions::opt_reestimation_reduce_lbd), 
    track_literal_occurrence(false),
    variableOccurrences(),
    allocator(), 
    clauses() {

}

ClauseDatabase::~ClauseDatabase() {
}

void ClauseDatabase::initOccurrenceTracking() {
    for (Clause* clause : clauses) {
        for (Lit lit : *clause) {
            variableOccurrences[var(lit)].push_back(clause);
        }
    }
    track_literal_occurrence = true;
}

void ClauseDatabase::stopOccurrenceTracking() {
    for (std::vector<Clause*>& occ : variableOccurrences) {
        occ.clear();
    }
    track_literal_occurrence = false;
}

// DYNAMIC NBLEVEL trick (see competition'09 Glucose companion paper)
void ClauseDatabase::reestimateClauseWeights(Trail& trail, std::vector<Clause*>& involved_clauses) {
    if (reestimationReduceLBD) {
        for (Clause* clause : involved_clauses) {
            if (clause->isLearnt()) {
                uint_fast16_t lbd = trail.computeLBD(clause->begin(), clause->end());
                if (lbd < clause->getLBD()) {
                    clause->setLBD(lbd);
                }
            }
        }
    }
}

/**
 * In order ot make sure that no clause is locked (reason to an asignment), 
 * do only call this method at decision level 0 and strengthen all clauses first
 **/
std::vector<Clause*> ClauseDatabase::reduce() { 
    Statistics::getInstance().solverReduceDBInc();

    std::vector<Clause*> learnts;
    copy_if(clauses.begin(), clauses.end(), std::back_inserter(learnts), [this](Clause* clause) { 
        return clause->getLBD() > persistentLBD && clause->size() > 2; 
    });
    std::sort(learnts.begin(), learnts.end(), [](Clause* c1, Clause* c2) { return c1->getLBD() > c2->getLBD(); });
    learnts.erase(learnts.begin() + (learnts.size() / 2), learnts.end());
    
    for (Clause* c : learnts) {
        removeClause(c);
    }

    Statistics::getInstance().solverRemovedClausesInc(learnts.size());

    return learnts;
}

/**
 * Make sure all references are updated after all clauses reside in a new adress space
 */
void ClauseDatabase::defrag() {
    clauses = allocator.defrag();
    for (std::vector<BinaryWatcher>& watcher : binaryWatchers) {
        watcher.clear();
    }
    for (Clause* clause : clauses) {
        if (clause->size() == 2) {
            binaryWatchers[toInt(~clause->first())].emplace_back(clause, clause->second());
            binaryWatchers[toInt(~clause->second())].emplace_back(clause, clause->first());
        }
    }
    if (track_literal_occurrence) {
        stopOccurrenceTracking();
        initOccurrenceTracking();
    }
}

}