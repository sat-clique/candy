#include <deque>
#include <unordered_map>
#include <vector>

#include "candy/core/Clause.h"
#include "candy/core/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/core/propagate/Propagate.h"
#include "candy/core/Certificate.h"
#include "candy/mtl/Stamp.h"
#include "candy/frontend/CLIOptions.h"

namespace Candy {

template <class TPropagate> class Subsumption { 

private:
    ClauseDatabase& clause_db;
    Trail& trail;
    TPropagate& propagator;
    Certificate& certificate;

    const uint16_t subsumption_lim;   // Do not check if subsumption against a clause larger than this. 0 means no limit.

public:         
    Subsumption(ClauseDatabase& clause_db_, Trail& trail_, TPropagate& propagator_, Certificate& certificate_) : 
        clause_db(clause_db_),
        trail(trail_),
        propagator(propagator_),
        certificate(certificate_),
        subsumption_lim(SubsumptionOptions::opt_subsumption_lim),
        queue(),
        abstractions(),
        bwdsub_assigns(0)
    {}

    std::deque<const Clause*> queue;
    std::unordered_map<const Clause*, uint64_t> abstractions;

    uint32_t bwdsub_assigns;

    void attachOccurences(Var v) {
        for (const Clause* clause : clause_db.refOccurences(v)) {
            attach(clause);
        }
    }

    void attach(const Clause* clause) {
        if (abstractions.count(clause) == 0 && !clause->isDeleted() && clause_db.isPersistent(clause)) {
            uint64_t abstraction = 0;
            for (Lit lit : *clause) {
                abstraction |= 1ull << (var(lit) % 64);
            }
            queue.push_back(clause);
            abstractions[clause] = abstraction;
        }
    }

    void clear() {
        queue.clear();
        abstractions.clear();
    }

    bool backwardSubsumptionCheck();

};

template <class TPropagate> bool Subsumption<TPropagate>::backwardSubsumptionCheck() {
    assert(trail.decisionLevel() == 0);

    Clause bwdsub_tmpunit({ lit_Undef });

    sort(queue.begin(), queue.end(), [](const Clause* c1, const Clause* c2) { return c1->size() < c2->size(); });
    
    while (queue.size() > 0 || bwdsub_assigns < trail.size()) {
        // Check top-level assignments by creating a dummy clause and placing it in the queue:
        if (queue.size() == 0 && bwdsub_assigns < trail.size()) {
            Lit l = trail[bwdsub_assigns++];
            bwdsub_tmpunit = Clause({l});
            abstractions.erase(&bwdsub_tmpunit);
            attach(&bwdsub_tmpunit);
        }

        const Clause* clause = queue.front();
        queue.pop_front();
        
        if (clause->isDeleted()) {
            continue;
        }
        
        assert(clause->size() > 1 || trail.value(clause->first()) == l_True); // Unit-clauses should have been propagated before this point.
        
        // Find best variable to scan:
        Var best = var(*std::min_element(clause->begin(), clause->end(), [this] (Lit l1, Lit l2) {
            return clause_db.numOccurences(var(l1)) < clause_db.numOccurences(var(l2));
        }));

        // Search all candidates:
        const uint64_t abstr = abstractions[clause];
        const std::vector<Clause*> occurences = clause_db.copyOccurences(best);
        for (const Clause* occurence : occurences) { // size might grow and that is ok
            if (occurence->isDeleted()) {
                continue;
            }
            if (occurence != clause && (subsumption_lim == 0 || occurence->size() < subsumption_lim)) {
                if ((abstr & ~abstractions[occurence]) != 0) continue;

                Lit l = clause->subsumes(*occurence);

                if (l != lit_Error) {
                    if (l == lit_Undef) { // remove:
                        Statistics::getInstance().solverSubsumedInc();
                    }
                    else { // strengthen:
                        Statistics::getInstance().solverDeletedInc();
                        std::vector<Lit> lits = occurence->except(l);
                        certificate.added(lits.begin(), lits.end());
                        
                        if (lits.size() == 1) {
                            if (!trail.newFact(lits.front()) || propagator.propagate() != nullptr) {
                                return false;
                            }
                        }
                        else {
                            Clause* new_clause = clause_db.createClause(lits);
                            propagator.attachClause(new_clause);
                            attach(new_clause);
                        }
                    }

                    propagator.detachClause(occurence);
                    clause_db.removeClause((Clause*)occurence);
                    abstractions.erase(occurence);
                    certificate.removed(occurence->begin(), occurence->end());
                }
            }
        }
    }

    return true;
}

}