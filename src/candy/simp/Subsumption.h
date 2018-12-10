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

    Stamp<uint8_t> touched;
    uint32_t n_touched;

public:         
    Subsumption(ClauseDatabase& clause_db_, Trail& trail_, TPropagate& propagator_, Certificate& certificate_) : 
        clause_db(clause_db_),
        trail(trail_),
        propagator(propagator_),
        certificate(certificate_),
        touched(),
        n_touched(0),
        reduced_literals(),
        subsumption_lim(SubsumptionOptions::opt_subsumption_lim),
        subsumption_queue(),
        abstraction(),
        bwdsub_assigns(0)
    {}

    std::vector<Lit> reduced_literals;

    uint16_t subsumption_lim;   // Do not check if subsumption against a clause larger than this. 0 means no limit.

    std::deque<Clause*> subsumption_queue;
    std::unordered_map<const Clause*, uint64_t> abstraction;
    uint32_t bwdsub_assigns;

    void gatherTouchedClauses() {
        if (n_touched == 0) {
            return;
        }
        
        for (unsigned int i = 0; i < touched.size(); i++) {
            if (touched[i]) {
                const std::vector<Clause*>& cs = clause_db.getOccurenceList(i);
                for (Clause* clause : cs) {
                    if (!clause->isDeleted()) {
                        subsumption_queue.push_back(clause);
                    }
                }
            }
        }

        touched.clear();
        n_touched = 0;
    }

    inline void touch(Var v) {
        n_touched++;
        touched.set(v);
    }

    inline bool hasTouchedClauses() {
        return n_touched > 0;
    }

    void attach(Clause* clause) {
        subsumption_queue.push_back(clause);
        calcAbstraction(clause);
    }

    void init(size_t nVars) {
        touched.grow(nVars);
        touched.clear();
        for (Clause* clause : clause_db) {
            attach(clause);
        }
    }

    void clear() {
        touched.clear();
        n_touched = 0;
        subsumption_queue.clear();
        abstraction.clear();
    }

    void calcAbstraction(const Clause* clause) {
        uint64_t clause_abstraction = 0;
        for (Lit lit : *clause) {
            clause_abstraction |= 1ull << (var(lit) % 64);
        }
        abstraction[clause] = clause_abstraction;
    }

    bool backwardSubsumptionCheck();

};

template <class TPropagate> bool Subsumption<TPropagate>::backwardSubsumptionCheck() {
    assert(trail.decisionLevel() == 0);

    Clause bwdsub_tmpunit({ lit_Undef });
    
    while (subsumption_queue.size() > 0 || bwdsub_assigns < trail.size()) {
        // Check top-level assignments by creating a dummy clause and placing it in the queue:
        if (subsumption_queue.size() == 0 && bwdsub_assigns < trail.size()) {
            Lit l = trail[bwdsub_assigns++];
            bwdsub_tmpunit = Clause({l});
            attach(&bwdsub_tmpunit);
        }

        const Clause* clause = subsumption_queue.front();
        subsumption_queue.pop_front();
        
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
        for (unsigned int i = 0; i < cs.size(); i++) { // size might grow and that is ok
            const Clause* csi = cs[i];
            if (csi->isDeleted()) {
                continue;
            }
            if (csi != clause && (subsumption_lim == 0 || csi->size() < subsumption_lim)) {
                if ((abstraction[clause] & ~abstraction[csi]) != 0) continue;

                Lit l = clause->subsumes(*csi);

                if (l != lit_Error) {
                    if (l == lit_Undef) { // remove:
                        Statistics::getInstance().solverSubsumedInc();
                        reduced_literals.insert(reduced_literals.end(), csi->begin(), csi->end());
                    }
                    else { // strengthen:
                        Statistics::getInstance().solverDeletedInc();
                        std::vector<Lit> lits = csi->except(l);
                        certificate.added(lits.begin(), lits.end());
                        
                        if (lits.size() == 1) {
                            reduced_literals.insert(reduced_literals.end(), csi->begin(), csi->end());
                            if (!trail.newFact(lits.front()) || propagator.propagate() != nullptr) {
                                return false;
                            }
                        }
                        else {
                            reduced_literals.push_back(l);
                            Clause* new_clause = clause_db.createClause(lits);
                            propagator.attachClause(new_clause);
                            attach(new_clause);
                        }
                    }

                    propagator.detachClause(csi);
                    clause_db.removeClause((Clause*)csi);
                    certificate.removed(csi->begin(), csi->end());
                }
            }
        }
    }

    return true;
}

}