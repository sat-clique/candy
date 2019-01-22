#include <vector>

#include "candy/core/clauses/Clause.h"
#include "candy/core/clauses/ClauseAllocator.h"
#include "candy/core/clauses/GlobalClauseAllocator.h"
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

    GlobalClauseAllocator* global_allocator;
 
    std::vector<Clause*> clauses; // Working set of problem clauses

    const unsigned int persistentLBD;
    const bool reestimationReduceLBD;

    bool track_literal_occurrence;    
    std::vector<std::vector<Clause*>> variableOccurrences;

    std::vector<std::vector<BinaryWatcher>> binaryWatchers;

public:
    /* analysis result is stored here */
	AnalysisResult result;

    ClauseDatabase() : 
        allocator(), global_allocator(nullptr), clauses(), 
        persistentLBD(ClauseDatabaseOptions::opt_persistent_lbd),
        reestimationReduceLBD(ClauseDatabaseOptions::opt_reestimation_reduce_lbd), 
        track_literal_occurrence(false),
        variableOccurrences(),
        binaryWatchers(), 
        result()
    { }

    ~ClauseDatabase() {
        allocator.free();
    }

    void initOccurrenceTracking() {
        for (Clause* clause : clauses) {
            for (Lit lit : *clause) {
                variableOccurrences[var(lit)].push_back(clause);
            }
        }
        track_literal_occurrence = true;
    }

    void stopOccurrenceTracking() {
        for (auto& occ : variableOccurrences) {
            occ.clear();
        }
        track_literal_occurrence = false;
    }

    void reestimateClauseWeights(Trail& trail, std::vector<Clause*>& involved_clauses) {
        if (reestimationReduceLBD) {
            for (Clause* clause : involved_clauses) {
                if (clause->isLearnt()) {
                    uint_fast16_t lbd = trail.computeLBD(clause->begin(), clause->end());
                    if (lbd < clause->getLBD()) {
                        clause->setLBD(lbd);
                    }
                }
            }
        }
    }

    inline void setGlobalClauseAllocator(GlobalClauseAllocator* global_allocator) {
        this->global_allocator = global_allocator;
        global_allocator->enroll();
        this->clauses = global_allocator->collect();
        for (std::vector<BinaryWatcher>& watcher : binaryWatchers) {
            watcher.clear();
        }
        for (Clause* clause : clauses) {
            if (clause->size() == 2) {
                binaryWatchers[toInt(~clause->first())].emplace_back(clause, clause->second());
                binaryWatchers[toInt(~clause->second())].emplace_back(clause, clause->first());
            }
        }
        if (track_literal_occurrence) {
            stopOccurrenceTracking();
            initOccurrenceTracking();
        }

    }

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

    Clause* strengthenClause(Clause* clause, Lit lit) {
        assert(clause->size() > 1);
        std::vector<Lit> literals;
        for (Lit literal : *clause) if (literal != lit) literals.push_back(literal);
        Clause* new_clause = createClause(literals.begin(), literals.end(), clause->getLBD());
        removeClause(clause);
        return new_clause;
    }

    Clause* persistClause(Clause* clause) {
        Clause* new_clause = createClause(clause->begin(), clause->end(), 0);
        removeClause(clause);
        return new_clause;
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

    /**
     * In order ot make sure that no clause is locked (reason to an asignment), 
     * do only call this method at decision level 0 and strengthen all clauses first
     **/
    std::vector<Clause*> reduce() { 
        Statistics::getInstance().solverReduceDBInc();

        std::vector<Clause*> learnts;
        copy_if(clauses.begin(), clauses.end(), std::back_inserter(learnts), [this](Clause* clause) { 
            return clause->getLBD() > persistentLBD && clause->size() > 2; 
        });
        std::sort(learnts.begin(), learnts.end(), [](Clause* c1, Clause* c2) { return c1->getLBD() > c2->getLBD(); });
        learnts.erase(learnts.begin() + (learnts.size() / 2), learnts.end());
        
        for (Clause* c : learnts) {
            removeClause(c);
        }

        Statistics::getInstance().solverRemovedClausesInc(learnts.size());

        return learnts;
    }

    /**
     * Make sure all references are updated after all clauses reside in a new adress space
     */
    void defrag() {
        if (global_allocator == nullptr) {
            clauses = allocator.reallocate();
        }
        else {
            clauses = global_allocator->import(allocator);
            allocator.reset(false);
        }

        for (std::vector<BinaryWatcher>& watcher : binaryWatchers) {
            watcher.clear();
        }
        for (Clause* clause : clauses) {
            if (clause->size() == 2) {
                binaryWatchers[toInt(~clause->first())].emplace_back(clause, clause->second());
                binaryWatchers[toInt(~clause->second())].emplace_back(clause, clause->first());
            }
        }
        if (track_literal_occurrence) {
            stopOccurrenceTracking();
            initOccurrenceTracking();
        }
    }

};

}

#endif