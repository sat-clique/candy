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
    ClauseAllocator allocator;
 
    std::vector<Clause*> clauses; // List of problem clauses

    const unsigned int persistentLBD;
    const bool reestimationReduceLBD;

    bool track_literal_occurrence;    
    std::vector<std::vector<Clause*>> variableOccurrences;

    std::vector<std::vector<BinaryWatcher>> binaryWatchers;

public:
    /* analysis result is stored here */
	AnalysisResult result;

    ClauseDatabase();
    ~ClauseDatabase();

    std::vector<Clause*> reduce();
    size_t cleanup();
    void defrag();

    void initOccurrenceTracking(size_t nVars);
    void stopOccurrenceTracking();

    void reestimateClauseWeights(Trail& trail, std::vector<Clause*>& involved_clauses);

    typedef std::vector<Clause*>::const_iterator const_iterator;

    inline const_iterator begin() const {
        return clauses.begin();
    }

    inline const_iterator end() const {
        return clauses.end();
    }

    inline unsigned int size() const {
        return clauses.size();
    }

    inline const Clause* operator [](int i) const {
        return clauses[i];
    }

    inline void grow(size_t nVars) {
        if (variableOccurrences.size() < nVars) {
            variableOccurrences.resize(nVars);
            binaryWatchers.resize(nVars*2);
        }
    }

    void setLearntClause(std::vector<Lit>& learnt_clause_, std::vector<Clause*>& involved_clauses_, unsigned int lbd_, unsigned int backtrack_level_) {
        result.setLearntClause(learnt_clause_, involved_clauses_, lbd_, backtrack_level_);
    }

    template<typename Iterator>
    Clause* createClause(Iterator begin, Iterator end, unsigned int lbd = 0) {
        // std::cout << "Creating clause " << lits;

        Clause* clause = new (allocator.allocate(std::distance(begin, end))) Clause(begin, end, lbd);
        clauses.push_back(clause);

        if (track_literal_occurrence) {
            for (Lit lit : *clause) {
                variableOccurrences[var(lit)].push_back(clause);
            }
        }

        if (clause->size() == 2) {
            binaryWatchers[toInt(~clause->first())].emplace_back(clause, clause->second());
            binaryWatchers[toInt(~clause->second())].emplace_back(clause, clause->first());
        }
        
        return clause;
    }

    void removeClause(Clause* clause) {
        // std::cout << "Removing clause " << *clause;

        clause->setDeleted();

        if (track_literal_occurrence) {
            for (Lit lit : *clause) {
                auto& list = variableOccurrences[var(lit)];
                list.erase(std::remove_if(list.begin(), list.end(), [clause](Clause* c){ return clause == c; }), list.end());
            }
        }

        if (clause->size() == 2) {
            std::vector<BinaryWatcher>& list0 = binaryWatchers[toInt(~clause->first())];
            std::vector<BinaryWatcher>& list1 = binaryWatchers[toInt(~clause->second())];
            list0.erase(std::remove_if(list0.begin(), list0.end(), [clause](BinaryWatcher w){ return w.clause == clause; }), list0.end());
            list1.erase(std::remove_if(list1.begin(), list1.end(), [clause](BinaryWatcher w){ return w.clause == clause; }), list1.end());
        }
    }

    inline size_t numOccurences(Var v) {
        return variableOccurrences[v].size();
    }

    inline std::vector<Clause*> copyOccurences(Var v) {
        return std::vector<Clause*>(variableOccurrences[v].begin(), variableOccurrences[v].end());
    }

    inline const std::vector<BinaryWatcher>& getBinaryWatchers(Lit lit) {
        return binaryWatchers[toInt(lit)];
    }

};

}

#endif