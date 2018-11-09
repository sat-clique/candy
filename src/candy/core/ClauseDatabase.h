#include <vector>

#include "candy/core/Clause.h"
#include "candy/core/ClauseAllocator.h"
#include "candy/core/Trail.h"
#include "candy/frontend/CLIOptions.h"

#ifndef CANDY_CLAUSE_DATABASE
#define CANDY_CLAUSE_DATABASE

namespace Candy {

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

    struct ClauseDeleted {
        explicit ClauseDeleted() { }
        inline bool operator()(const Clause* cr) const {
            return cr->isDeleted();
        }
    };

    // clause activity heuristic
    double cla_inc; // Amount to bump next clause with.
    double clause_decay;

    uint_fast8_t persistentLBD;
    bool track_literal_occurrence;
    
    OccLists<Var, Clause*, ClauseDeleted> variableOccurrences;

public:
    ClauseAllocator allocator;
 
    std::vector<Clause*> clauses; // List of problem clauses

    ClauseDatabase();
    ~ClauseDatabase();

    void reduce();
    void defrag();
    void freeMarkedClauses();

    void initOccurrenceTracking(size_t nVars);
    void stopOccurrenceTracking();

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

    Clause* createClause(Cl& lits, unsigned int lbd = 0) {
        // std::cout << "Creating clause " << lits;

        Clause* clause = new (allocator.allocate(lits.size())) Clause(lits);
        if (lbd > 0) {
            clause->setLBD(lbd);
            clause->setLearnt(true);
            bumpActivity(*clause);
        }
        clauses.push_back(clause);

        if (track_literal_occurrence) {
            for (Lit lit : *clause) {
                variableOccurrences[var(lit)].push_back(clause);
            }
        }
        
        return clause;
    }

    void removeClause(Clause* clause) {
        // std::cout << "Removing clause " << *clause;

        clause->setDeleted();

        if (track_literal_occurrence) {
            for (Lit lit : *clause) variableOccurrences.smudge(var(lit));
        }
    }

    void strengthenClause(Clause* clause, Lit lit) {
        // std::cout << "Strengthening literal " << lit << " in clause " << *clause;

        clause->strengthen(lit);

        if (track_literal_occurrence) {
            variableOccurrences.remove(var(lit), clause);
        }

        if (clause->size() < 2) {
            removeClause(clause); 
        }
    }

    inline const std::vector<Clause*>& getOccurenceList(Var v) {
        return variableOccurrences.lookup(v);
    }

    void cleanup() {
        auto new_end = std::remove_if(clauses.begin(), clauses.end(), [this](Clause* c) { return c->isDeleted(); });
        clauses.erase(new_end, clauses.end());

        if (track_literal_occurrence) {
            variableOccurrences.cleanAll();
        }
    }

    void bumpActivities(std::vector<Clause*>& involved_clauses);

    inline void decayActivity() {
        cla_inc *= (1 / clause_decay);
    }

    inline void bumpActivity(Clause& c) {
        if ((c.activity() += static_cast<float>(cla_inc)) > 1e20f) {
            rescaleActivity();
        }
    }

    void rescaleActivity();

};

}

#endif