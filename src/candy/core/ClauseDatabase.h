#include <vector>

#include "candy/core/Clause.h"
#include "candy/core/ClauseAllocator.h"
#include "candy/core/Trail.h"
#include "candy/utils/Options.h"

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