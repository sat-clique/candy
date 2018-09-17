#include <deque>
#include <unordered_map>
#include <vector>

#include "candy/core/Clause.h"
#include "candy/core/Trail.h"

namespace Candy {

class Subsumption {
    struct ClauseDeleted {
        explicit ClauseDeleted() { }
        inline bool operator()(const Clause* cr) const {
            return cr->isDeleted();
        }
    };

public:         
    Subsumption(Trail& trail) : 
        trail(trail),
        subsumption_lim(SimpSolverOptions::opt_subsumption_lim),
        occurs(ClauseDeleted()),
        subsumption_queue(),
        subsumption_queue_contains(),
        strengthened_clauses(),
        strengthened_sizes(),
        abstraction(),
        bwdsub_assigns(0)
    {}

    Trail& trail;

    uint16_t subsumption_lim;   // Do not check if subsumption against a clause larger than this. 0 means no limit.

    std::deque<Clause*> subsumption_queue;
    OccLists<Var, Clause*, ClauseDeleted> occurs;
    std::unordered_map<Clause*, char> subsumption_queue_contains;
    std::vector<Clause*> strengthened_clauses;
    std::unordered_map<Clause*, size_t> strengthened_sizes;
    std::unordered_map<Clause*, uint64_t> abstraction;
    uint32_t bwdsub_assigns;

    void subsumptionQueueProtectedPush(Clause* clause);
    Clause* subsumptionQueueProtectedPop();

    void attach(Clause* clause);
    void detach(Clause* clause, Lit lit, bool strict);

    void calcAbstraction(Clause* clause);

    void rememberSizeBeforeStrengthening(Clause* clause);

    bool strengthenClause(Clause* cr, Lit l);
    bool backwardSubsumptionCheck();

};

}