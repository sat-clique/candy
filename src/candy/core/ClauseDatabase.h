#include <vector>

#include "candy/core/Clause.h"
#include "candy/core/ClauseAllocator.h"
#include "candy/core/Trail.h"
#include "candy/utils/Options.h"

#ifndef CANDY_CLAUSE_DATABASE
#define CANDY_CLAUSE_DATABASE

namespace Candy {

namespace ClauseDatabaseOptions {
    extern Glucose::IntOption opt_persistent_lbd;
    extern Glucose::IntOption opt_lb_lbd_frozen_clause;
    extern Glucose::DoubleOption opt_clause_decay;
}

class ClauseDatabase {
private:
    struct reduceDB_lt {
        reduceDB_lt() {
        }

        bool operator()(Clause* x, Clause* y) {
    //        // XMiniSat paper, alternate ordering
    //        const uint8_t reduceOnSizeSize = 12;
    //        uint32_t w1 = x->size() < reduceOnSizeSize : x->size() : x->size() + x->getLBD();
    //        uint32_t w2 = y->size() < reduceOnSizeSize : y->size() : y->size() + y->getLBD();
    //        return w1 > w2 || (w1 == w2 && x->activity() < y->activity());
            return x->getLBD() > y->getLBD() || (x->getLBD() == y->getLBD() && x->activity() < y->activity());
        }
    };

    Trail& trail;

    uint_fast16_t persistentLBD;
    uint_fast16_t lbLBDFrozenClause;

    // clause activity heuristic
    double cla_inc; // Amount to bump next clause with.
    double clause_decay;

public:

    ClauseAllocator allocator;
 
    std::vector<Clause*> clauses; // List of problem clauses.

    std::vector<Clause*> removed; // List of clauses to be removed

    ClauseDatabase(Trail& trail_);
    ~ClauseDatabase();

    void reduce();
    void defrag();
    void freeMarkedClauses();

    typedef std::vector<Clause*>::iterator iterator;
    typedef std::vector<Clause*>::const_iterator const_iterator;
    typedef std::vector<Clause*>::reverse_iterator reverse_iterator;
    typedef std::vector<Clause*>::const_reverse_iterator const_reverse_iterator;

    inline const_iterator begin() const {
        return clauses.begin();
    }

    inline const_iterator end() const {
        return clauses.end();
    }

    inline iterator begin() {
        return clauses.begin();
    }

    inline iterator end() {
        return clauses.end();
    }

    inline const_reverse_iterator rbegin() const {
        return clauses.rbegin();
    }

    inline const_reverse_iterator rend() const {
        return clauses.rend();
    }

    inline reverse_iterator rbegin() {
        return clauses.rbegin();
    }

    inline reverse_iterator rend() {
        return clauses.rend();
    }

    inline unsigned int size() const {
        return clauses.size();
    }

    Clause* newClause(Cl& lits) {
        Clause* clause = new (allocator.allocate(lits.size())) Clause(lits);
        clauses.push_back(clause);
        return clause;
    }

    void removeClause(Clause* clause, bool strict = false) {
        removed.push_back(clause);
        clause->setDeleted();
        if (strict) {
            clauses.erase(std::remove(clauses.begin(), clauses.end(), clause), clauses.end());    
        }
    }

    void cleanup() {
        auto new_end = std::remove_if(clauses.begin(), clauses.end(), [this](Clause* c) { return c->isDeleted(); });
        clauses.erase(new_end, clauses.end());
    }

    size_t nLearnts() const;

    void updateClauseActivitiesAndLBD(std::vector<Clause*>& involved_clauses, unsigned int learnt_lbd);

    inline uint_fast16_t getPersistentLBD() {// access is temporary
        return persistentLBD;
    }

    inline void claDecayActivity() {
        cla_inc *= (1 / clause_decay);
    }

    inline void claBumpActivity(Clause& c) {
        if ((c.activity() += static_cast<float>(cla_inc)) > 1e20f) {
            claRescaleActivity();
        }
    }

    void claRescaleActivity();

};

}

#endif