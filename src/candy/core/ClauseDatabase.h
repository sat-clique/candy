#include <vector>

#include "candy/core/Clause.h"
#include "candy/core/ClauseAllocator.h"
#include "candy/core/Trail.h"
#include "candy/frontend/CLIOptions.h"

#ifndef CANDY_CLAUSE_DATABASE
#define CANDY_CLAUSE_DATABASE

namespace Candy {

struct AnalysisResult {

	AnalysisResult() : 
		nConflicts(0), learnt_clause(), involved_clauses(), lbd(0), backtrack_level(0)
	{ }

	uint64_t nConflicts;
	std::vector<Lit> learnt_clause;
	std::vector<Clause*> involved_clauses;

	unsigned int lbd;
    unsigned int backtrack_level;

	void clear() {
		learnt_clause.clear();
		involved_clauses.clear();
        lbd = 0;
        backtrack_level = 0;
	}

    void setLearntClause(std::vector<Lit>& learnt_clause_) {
        learnt_clause.swap(learnt_clause_);
        involved_clauses.clear();
        lbd = 0;
        backtrack_level = 0;
    }

    void setLearntClause(std::vector<Lit>& learnt_clause_, std::vector<Clause*>& involved_clauses_, unsigned int lbd_, unsigned int backtrack_level_) {
        nConflicts++;
        learnt_clause.swap(learnt_clause_);
        involved_clauses.swap(involved_clauses_);
        lbd = lbd_;
        backtrack_level = backtrack_level_;
    }

};

struct BinaryWatcher {
    Clause* clause;
    Lit other;

    BinaryWatcher(Clause* _clause, Lit _other)
        : clause(_clause), other(_other) { }

};

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

    std::vector<std::vector<BinaryWatcher>> binaryWatchers;

    void bumpActivities(std::vector<Clause*>& involved_clauses) {
        for (Clause* clause : involved_clauses) {
            bumpActivity(*clause);
        }
    }

    void rescaleActivity() {
        for (Clause* clause : clauses) {
            clause->activity() *= 1e-20f;
        }
        cla_inc *= 1e-20;
    }

    void decayActivity() {
        cla_inc *= (1 / clause_decay);
    }

    void bumpActivity(Clause& c) {
        if ((c.activity() += static_cast<float>(cla_inc)) > 1e20f) {
            rescaleActivity();
        }
    }

	// DYNAMIC NBLEVEL trick (see competition'09 Glucose companion paper)
    void reduceLBDs(Trail& trail, std::vector<Clause*>& involved_clauses) {
        for (Clause* clause : involved_clauses) {
            if (clause->isLearnt()) {
                uint_fast16_t nblevels = trail.computeLBD(clause->begin(), clause->end());
                if (nblevels + 1 < clause->getLBD()) {
                    clause->setLBD(nblevels); // improve the LBD
                    clause->setFrozen(true); // Seems to be interesting, keep it for the next round
                }
            }
        }
    }

public:
    ClauseAllocator allocator;

    /* analysis result is stored here */
	AnalysisResult result;
 
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

    inline const Clause* operator [](int i) const {
        return clauses[i];
    }

    inline void grow(size_t nVars) {
        if (variableOccurrences.size() < nVars+1) {
            variableOccurrences.init(nVars+1);
            binaryWatchers.resize(nVars*2+2);
        }
    }

    void setLearntClause(std::vector<Lit>& learnt_clause_) {
        result.setLearntClause(learnt_clause_);
    }

    void setLearntClause(std::vector<Lit>& learnt_clause_, std::vector<Clause*>& involved_clauses_, unsigned int lbd_, unsigned int backtrack_level_) {
        result.setLearntClause(learnt_clause_, involved_clauses_, lbd_, backtrack_level_);
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

        if (clause->size() == 2) {
            binaryWatchers[~clause->first()].emplace_back(clause, clause->second());
            binaryWatchers[~clause->second()].emplace_back(clause, clause->first());
        }
        
        return clause;
    }

    void removeClause(Clause* clause) {
        // std::cout << "Removing clause " << *clause;

        clause->setDeleted();

        if (track_literal_occurrence) {
            for (Lit lit : *clause) variableOccurrences.smudge(var(lit));
        }

        if (clause->size() == 2) {
            std::vector<BinaryWatcher>& list0 = binaryWatchers[~clause->first()];
            std::vector<BinaryWatcher>& list1 = binaryWatchers[~clause->second()];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [clause](BinaryWatcher w){ return w.clause == clause; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [clause](BinaryWatcher w){ return w.clause == clause; }), list1.end());
        }
    }

    void strengthenClause(Clause* clause, Lit lit) {
        // std::cout << "Strengthening literal " << lit << " in clause " << *clause;

        if (clause->size() == 2) {
            removeClause(clause); 
        }
        else if (track_literal_occurrence) {
            variableOccurrences.remove(var(lit), clause);
        }

        clause->strengthen(lit);

        if (clause->size() == 2) {
            binaryWatchers[~clause->first()].emplace_back(clause, clause->second());
            binaryWatchers[~clause->second()].emplace_back(clause, clause->first());
        }
    }

    inline const std::vector<Clause*>& getOccurenceList(Var v) {
        return variableOccurrences.lookup(v);
    }

    inline const std::vector<BinaryWatcher>& getBinaryWatchers(Lit lit) {
        return binaryWatchers[lit];
    }

    void cleanup() {
        auto new_end = std::remove_if(clauses.begin(), clauses.end(), [this](Clause* c) { return c->isDeleted(); });
        clauses.erase(new_end, clauses.end());

        if (track_literal_occurrence) {
            variableOccurrences.cleanAll();
        }
    }

    void reestimateClauseWeights(Trail& trail, std::vector<Clause*>& involved_clauses) {
		reduceLBDs(trail, involved_clauses);
        bumpActivities(involved_clauses); 
        decayActivity();
    }

};

}

#endif