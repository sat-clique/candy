#include "candy/simp/Subsumption.h"
#include "candy/core/Clause.h"
#include "candy/utils/Options.h"

#include <assert.h>

namespace Candy {

namespace SubsumptionOptions {
const char* _cat = "SIMP";

IntOption opt_subsumption_lim(_cat, "sub-lim", "Do not check if subsumption against a clause larger than this.", 1000, IntRange(0, INT32_MAX));
}

void Subsumption::subsumptionQueueProtectedPush(Clause* clause) {
    if (!subsumption_queue_contains[clause]) {
        subsumption_queue.push_back(clause);
        subsumption_queue_contains[clause] = true;
    }
}

Clause* Subsumption::subsumptionQueueProtectedPop() {
    Clause* clause = subsumption_queue.front();
    subsumption_queue.pop_front();
    subsumption_queue_contains[clause] = false;
    return clause;
}

void Subsumption::gatherTouchedClauses() {
    if (n_touched == 0) {
        return;
    }
    
    for (unsigned int i = 0; i < touched.size(); i++) {
        if (touched[i]) {
            const vector<Clause*>& cs = clause_db.getOccurenceList(i);
            for (Clause* c : cs) {
                if (!c->isDeleted()) {
                    subsumptionQueueProtectedPush(c);
                }
            }
        }
    }

    touched.clear();
    n_touched = 0;
}

void Subsumption::attach(Clause* clause) {
    subsumption_queue.push_back(clause);
    subsumption_queue_contains[clause] = true;
    calcAbstraction(clause);
}

void Subsumption::init(size_t nVars) {
    touched.grow(nVars);
    touched.clear();
    for (Clause* clause : clause_db) {
        attach(clause);
    }
}

void Subsumption::clear() {
    touched.clear();
    n_touched = 0;
    subsumption_queue.clear();
    abstraction.clear();
}

void Subsumption::calcAbstraction(Clause* clause) {
    uint64_t clause_abstraction = 0;
    for (Lit lit : *clause) {
        clause_abstraction |= 1ull << (var(lit) % 64);
    }
    abstraction[clause] = clause_abstraction;
}

bool Subsumption::strengthenClause(Clause* clause, Lit l) {
    assert(trail.decisionLevel() == 0);
    
    subsumptionQueueProtectedPush(clause);

    reduced_literals.push_back(l);
    propagator.detachClause(clause, true);
    certificate.added(clause->begin(), clause->end());
    clause_db.strengthenClause(clause, l);
    
    if (clause->size() == 1) {
        reduced_literals.push_back(clause->first());
        return trail.newFact(clause->first()) && propagator.propagate() == nullptr;
    }
    else {
        propagator.attachClause(clause);
        calcAbstraction(clause);
        return true;
    }
}

bool Subsumption::backwardSubsumptionCheck() {
    assert(trail.decisionLevel() == 0);

    Clause bwdsub_tmpunit({ lit_Undef });
    
    while (subsumption_queue.size() > 0 || bwdsub_assigns < trail.size()) {
        // Check top-level assignments by creating a dummy clause and placing it in the queue:
        if (subsumption_queue.size() == 0 && bwdsub_assigns < trail.size()) {
            Lit l = trail[bwdsub_assigns++];
            bwdsub_tmpunit[0] = l;
            abstraction[&bwdsub_tmpunit] = 1ull << (var(l) % 64);
            subsumption_queue.push_back(&bwdsub_tmpunit);
        }

        const Clause* clause = subsumptionQueueProtectedPop();
        
        if (clause->isDeleted()) {
            continue;
        }
        
        assert(clause->size() > 1 || trail.value(clause->first()) == l_True); // Unit-clauses should have been propagated before this point.
        
        // Find best variable to scan:
        Var best = var(*std::min_element(clause->begin(), clause->end(), [this] (Lit l1, Lit l2) {
            return clause_db.getOccurenceList(var(l1)).size() < clause_db.getOccurenceList(var(l2)).size();
        }));

        // Search all candidates:
        const std::vector<Clause*>& cs = clause_db.getOccurenceList(best);
        for (unsigned int i = 0; i < cs.size(); i++) {
            const Clause* csi = cs[i];
            if (csi != clause && (subsumption_lim == 0 || csi->size() < subsumption_lim)) {
                if ((abstraction[clause] & ~abstraction[csi]) != 0) continue;

                Lit l = clause->subsumes(*csi);

                if (l == lit_Undef) {
                    Statistics::getInstance().solverSubsumedInc();
                    certificate.removed(csi->begin(), csi->end());
                    propagator.detachClause(csi, true);
                    if (trail.locked(csi)) {
                        trail.vardata[var(csi->first())].reason = nullptr;
                    }
                    clause_db.removeClause((Clause*)csi);
                    reduced_literals.insert(reduced_literals.end(), csi->begin(), csi->end());
                }
                else if (l != lit_Error) {
                    Statistics::getInstance().solverDeletedInc();
                    // this might modifiy occurs ...
                    if (!strengthenClause((Clause*)csi, ~l)) {
                        return false;
                    }
                    // ... occurs modified, so check candidate at index i again:
                    if (var(l) == best) {
                        i--;
                    }
                }
            }
        }
    }

    return true;
}

}