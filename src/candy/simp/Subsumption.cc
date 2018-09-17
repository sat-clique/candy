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

void Subsumption::attach(Clause* clause) {
    subsumption_queue.push_back(clause);
    subsumption_queue_contains[clause] = true;
    for (Lit lit : *clause) {
        occurs[var(lit)].push_back(clause);
    }
    calcAbstraction(clause);
}

void Subsumption::detach(Clause* clause, Lit lit, bool strict) {
    if (strict) {
        occurs[var(lit)].erase(std::remove(occurs[var(lit)].begin(), occurs[var(lit)].end(), clause), occurs[var(lit)].end());
    }
    else {
        occurs.smudge(var(lit));
    }
}

void Subsumption::calcAbstraction(Clause* clause) {
    uint64_t clause_abstraction = 0;
    for (Lit lit : *clause) {
        clause_abstraction |= 1ull << (var(lit) % 64);
    }
    abstraction[clause] = clause_abstraction;
}

void Subsumption::rememberSizeBeforeStrengthening(Clause* clause) {
    if (strengthened_sizes.count(clause) == 0) { // used to cleanup pages in clause-pool
        strengthened_sizes[clause] = clause->size();
        strengthened_clauses.push_back(clause);
    }
}

bool Subsumption::strengthenClause(Clause* clause, Lit l) {
    assert(trail.decisionLevel() == 0);
    
    subsumptionQueueProtectedPush(clause);
    rememberSizeBeforeStrengthening(clause);        
    
    propagator.detachClause(clause, true);
    clause->strengthen(l);

    certificate.added(clause->begin(), clause->end());

    detach(clause, l, true);
    removed.push_back(clause);
    
    if (clause->size() == 1) {
        Lit unit = clause->first();
        detach(clause, unit, true);
        clause->setDeleted();

        if (trail.value(unit) == l_Undef) {
            trail.uncheckedEnqueue(unit);
            return propagator.propagate() == nullptr;
        }
        else if (trail.value(unit) == l_False) {
            return false;
        }
        else {
            trail.vardata[var(unit)].reason = nullptr;
            trail.vardata[var(unit)].level = 0;
            return true;
        }
    }
    else {
        propagator.attachClause(clause);

        calcAbstraction(clause);

        assert(clause->size() > 1);
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

        Clause* clause = subsumptionQueueProtectedPop();
        
        if (clause->isDeleted()) {
            continue;
        }
        
        assert(clause->size() > 1 || trail.value(clause->first()) == l_True); // Unit-clauses should have been propagated before this point.
        
        // Find best variable to scan:
        Var best = var(*std::min_element(clause->begin(), clause->end(), [this] (Lit l1, Lit l2) {
            return occurs[var(l1)].size() < occurs[var(l2)].size();
        }));

        // Search all candidates:
        std::vector<Clause*>& cs = occurs.lookup(best);
        for (unsigned int i = 0; i < cs.size(); i++) {
            Clause* csi = cs[i];
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
                    csi->setDeleted();
                    // this->removeClause(csi, true);
                    for (Lit lit : *csi) detach(csi, lit, false);
                    removed.push_back(csi);
                }
                else if (l != lit_Error) {
                    Statistics::getInstance().solverDeletedInc();
                    // this might modifiy occurs ...
                    if (!strengthenClause(csi, ~l)) {
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