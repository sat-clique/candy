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

    ClauseAllocator allocator;
 
    std::vector<Clause*> clauses; // List of problem clauses

    // clause activity heuristic
    double cla_inc; // Amount to bump next clause with.
    double clause_decay;

    uint_fast8_t persistentLBD;
    bool track_literal_occurrence;
    
    OccLists<Var, Clause*, ClauseDeleted> variableOccurrences;

    std::vector<std::vector<BinaryWatcher>> binaryWatchers;

    void bumpActivities(std::vector<Clause*>& involved_clauses);
    void bumpActivity(Clause& c);
    void rescaleActivity();
    void decayActivity();
    void reduceLBDs(Trail& trail, std::vector<Clause*>& involved_clauses);

public:
    /* analysis result is stored here */
	AnalysisResult result;

    ClauseDatabase();
    ~ClauseDatabase();

    void reduce();
    void cleanup();
    void defrag();

    void initOccurrenceTracking(size_t nVars);
    void stopOccurrenceTracking();

    void reestimateClauseWeights(Trail& trail, std::vector<Clause*>& involved_clauses);

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
            for (Lit lit : *clause) {
                //variableOccurrences.smudge(var(lit));
                auto& list = variableOccurrences[var(lit)];
                list.erase(std::remove_if(list.begin(), list.end(), [clause](Clause* c){ return clause == c; }), list.end());
            }
        }

        if (clause->size() == 2) {
            std::vector<BinaryWatcher>& list0 = binaryWatchers[~clause->first()];
            std::vector<BinaryWatcher>& list1 = binaryWatchers[~clause->second()];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [clause](BinaryWatcher w){ return w.clause == clause; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [clause](BinaryWatcher w){ return w.clause == clause; }), list1.end());
        }
    }

    inline const std::vector<Clause*>& getOccurenceList(Var v) {
        return variableOccurrences.lookup(v);
    }

    inline const std::vector<BinaryWatcher>& getBinaryWatchers(Lit lit) {
        return binaryWatchers[lit];
    }

};

}

#endif