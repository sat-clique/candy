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
    uint64_t solves, starts, decisions, rnd_decisions;
    uint64_t nbstopsrestarts, nbstopsrestartssame, lastblockatrestart;
    uint64_t dec_vars, clauses_literals, max_literals, tot_literals;
    double totalTime4Sat, totalTime4Unsat;
    int nbSatCalls, nbUnsatCalls;

public:
    SolverStatistics();
    virtual ~SolverStatistics();

    void printIncrementalStats(uint64_t conflicts, uint64_t propagations);
    void printIntermediateStats(int trail, int clauses, int learnts, uint64_t conflicts);
    void printFinalStats(double cpu_time, double mem_used, uint64_t conflicts, uint64_t propagations);

    inline void incDecVars(int amount = 1) {
        dec_vars += amount;
    }

    inline void incClausesLiterals(int amount) {
        clauses_literals += amount;
    }

    inline void incRemovedClauses(int amount) {
        nbRemovedClauses += amount;
    }

    inline void incSumDecisionLevels(int amount) {
        sumDecisionLevels += amount;
    }

    inline void incDecisions() {
        ++decisions;
    }

    inline void incSolves() {
        ++solves;
    }

    inline void incNBSatCalls() {
        ++nbSatCalls;
    }

    inline void incNBUnsatCalls() {
        ++nbUnsatCalls;
    }

    inline void incTotalTime4Sat(int amount) {
        totalTime4Sat += amount;
    }

    inline void incTotalTime4Unsat(int amount) {
        totalTime4Unsat += amount;
    }

    inline void incReducedClauses() {
        ++nbReducedClauses;
    }

    inline void incNBReduceDB() {
        ++nbReduceDB;
    }

    inline void incRndDecisions() {
        ++rnd_decisions;
    }

    inline void incMaxLiterals(int amount) {
        max_literals += amount;
    }

    inline void incTotLiterals(int amount) {
        tot_literals += amount;
    }

    inline void incStarts() {
        ++starts;
    }

    inline void incNBUn() {
        ++nbUn;
    }

    inline void incNBDL2() {
        ++nbDL2;
    }

    inline void incNBBin() {
        ++nbBin;
    }

    inline void incNBStopsRestarts() {
        ++nbstopsrestarts;
    }

    inline void incNBStopsRestartsSame() {
        ++nbstopsrestartssame;
    }

    inline void saveLastBlockAtRestart() {
        lastblockatrestart = starts;
    }
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_SOLVERSTATISTICS_H_ */
