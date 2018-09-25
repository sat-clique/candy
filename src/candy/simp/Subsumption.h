#include <deque>
#include <unordered_map>
#include <vector>

#include "candy/core/Clause.h"
#include "candy/core/ClauseDatabase.h"
#include "candy/core/Trail.h"
#include "candy/core/Propagate.h"
#include "candy/core/Certificate.h"
#include "candy/core/Stamp.h"
#include "candy/utils/Options.h"

namespace Candy {

namespace SubsumptionOptions {
using namespace Glucose;

extern const char* _cat;

extern IntOption opt_subsumption_lim;
}

class Subsumption {
private:
    struct ClauseDeleted {
        explicit ClauseDeleted() { }
        inline bool operator()(const Clause* cr) const {
            return cr->isDeleted();
        }
    };

    ClauseDatabase& clause_db;
    Trail& trail;
    Propagate& propagator;
    Certificate& certificate;

    Stamp<uint8_t> touched;
    uint32_t n_touched;

public:         
    Subsumption(ClauseDatabase& clause_db_, Trail& trail_, Propagate& propagator_, Certificate& certificate_) : 
        clause_db(clause_db_),
        trail(trail_),
        propagator(propagator_),
        certificate(certificate_),
        touched(),
        n_touched(0),
        reduced_literals(),
        subsumption_lim(SubsumptionOptions::opt_subsumption_lim),
        occurs(ClauseDeleted()),
        subsumption_queue(),
        subsumption_queue_contains(),
        abstraction(),
        bwdsub_assigns(0)
    {}

    std::vector<Lit> reduced_literals;

    uint16_t subsumption_lim;   // Do not check if subsumption against a clause larger than this. 0 means no limit.

    std::deque<Clause*> subsumption_queue;
    OccLists<Var, Clause*, ClauseDeleted> occurs;
    std::unordered_map<Clause*, char> subsumption_queue_contains;
    std::unordered_map<Clause*, uint64_t> abstraction;
    uint32_t bwdsub_assigns;

    void subsumptionQueueProtectedPush(Clause* clause);
    Clause* subsumptionQueueProtectedPop();

    void gatherTouchedClauses();

    inline void touch(Var v) {
        n_touched++;
        touched.set(v);
    }

    inline bool hasTouchedClauses() {
        return n_touched > 0;
    }

    void init(size_t nVars);
    void clear();

    void attach(Clause* clause);
    void detach(Clause* clause, Lit lit, bool strict);

    void calcAbstraction(Clause* clause);

    bool strengthenClause(Clause* cr, Lit l);
    bool backwardSubsumptionCheck();

};

}