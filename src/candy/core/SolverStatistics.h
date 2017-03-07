/*
 * SolverStatistics.h
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_SOLVERSTATISTICS_H_
#define SRC_CANDY_CORE_SOLVERSTATISTICS_H_

#include <cstdint>
#include <cstdio>

namespace Candy {

class SolverStatistics {
    uint64_t sumDecisionLevels;
    uint64_t nbRemovedClauses, nbReducedClauses, nbDL2, nbBin, nbUn, nbReduceDB;
    uint64_t solves, starts, decisions, rnd_decisions, propagations, conflicts;
    uint64_t conflictsRestarts, nbstopsrestarts, nbstopsrestartssame, lastblockatrestart;
    uint64_t dec_vars, clauses_literals, learnts_literals, max_literals, tot_literals;
    double totalTime4Sat, totalTime4Unsat;
    int nbSatCalls, nbUnsatCalls;

public:
    SolverStatistics();
    virtual ~SolverStatistics();

    void printIncrementalStats();
    void printIntermediateStats(int trail, int clauses, int learnts);
    void printFinalStats(double cpu_time, double mem_used);

    inline int64_t getTotalLiterals() {
        return learnts_literals + clauses_literals;
    }

    inline int64_t getConflicts() {
        return conflicts;
    }

    inline int64_t getPropagations() {
        return propagations;
    }

    inline int64_t getConflictsRestarts() {
        return conflictsRestarts;
    }

    inline void incDecVars(int amount = 1) {
        dec_vars += amount;
    }

    inline int getDecVars() {
        return dec_vars;
    }

    inline void incLearntsLiterals(int amount) {
        learnts_literals += amount;
    }

    inline void incClausesLiterals(int amount) {
        clauses_literals += amount;
    }

    inline void incPropagations(int amount) {
        propagations += amount;
    }

    inline void incRemovedClauses(int amount) {
        nbRemovedClauses += amount;
    }

    inline void incSumDecisionLevels(int amount) {
        sumDecisionLevels += amount;
    }

    inline void incConflicts(int amount = 1) {
        conflicts += amount;
    }

    inline void incDecisions(int amount = 1) {
        decisions += amount;
    }

    inline void incSolves(int amount = 1) {
        solves += amount;
    }

    inline void incNBSatCalls(int amount = 1) {
        nbSatCalls += amount;
    }

    inline void incNBUnsatCalls(int amount = 1) {
        nbUnsatCalls += amount;
    }

    inline void incTotalTime4Sat(int amount) {
        totalTime4Sat += amount;
    }

    inline void incTotalTime4Unsat(int amount) {
        totalTime4Unsat += amount;
    }

    inline void incConflictsRestarts(int amount = 1) {
        conflictsRestarts += amount;
    }

    inline void incReducedClauses(int amount = 1) {
        nbReducedClauses += amount;
    }

    inline void incNBReduceDB(int amount = 1) {
        nbReduceDB += amount;
    }

    inline void incRndDecisions(int amount = 1) {
        rnd_decisions += amount;
    }

    inline void incMaxLiterals(int amount = 1) {
        max_literals += amount;
    }

    inline void incTotLiterals(int amount = 1) {
        tot_literals += amount;
    }

    inline void incStarts(int amount = 1) {
        starts += amount;
    }

    inline void incNBUn(int amount = 1) {
        nbUn += amount;
    }

    inline void incNBDL2(int amount = 1) {
        nbDL2 += amount;
    }

    inline void incNBBin(int amount = 1) {
        nbBin += amount;
    }

    inline void incNBStopsRestarts(int amount = 1) {
        nbstopsrestarts += amount;
    }

    inline void incNBStopsRestartsSame(int amount = 1) {
        nbstopsrestartssame += amount;
    }

    inline void saveLastBlockAtRestart() {
        lastblockatrestart = starts;
    }
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_SOLVERSTATISTICS_H_ */
