/*
 * SolverStatistics.h
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_STATISTICS_H_
#define SRC_CANDY_CORE_STATISTICS_H_

#include <cstdint>
#include <cstdio>
#include <cassert>
#include <vector>

#define SOLVER_STATS
#define RESTART_STATS
#define LEARNTS_STATS
//#define INCREMENTAL_STATS
//#define ALLOCATOR_STATS

namespace Candy {

class Statistics {
#ifdef SOLVER_STATS
    uint64_t decisions, rnd_decisions, dec_vars, max_literals, tot_literals;
#endif// SOLVER_STATS
#ifdef RESTART_STATS
    uint64_t starts, nbstopsrestarts, nbstopsrestartssame, lastblockatrestart;
#endif// SOLVER_STATS
#ifdef LEARNTS_STATS
    uint64_t nbRemovedClauses, nbReducedClauses, nbDL2, nbBin, nbUn, nbReduceDB;
#endif// LEARNTS_STATS
#ifdef INCREMENTAL_STATS
    double totalTime4Sat, totalTime4Unsat;
    int nbSatCalls, nbUnsatCalls;
#endif// INCREMENTAL_STATS
#ifdef ALLOCATOR_STATS
    std::vector<uint32_t> stats_pool_allocd, stats_pool_hwm;
    uint32_t stats_number_of_pools, stats_xxl_pool_allocd, stats_beyond_mallocd;
    uint32_t stats_xxl_pool_hwm, stats_beyond_hwm;
#endif// ALLOCATOR_STATS
    char stats;

private:
    Statistics();

public:
    inline static Statistics& getInstance() {
        static Statistics statistics;
        return statistics;
    }
    ~Statistics();

    void printIncrementalStats(uint64_t conflicts, uint64_t propagations);
    void printIntermediateStats(int trail, int clauses, int learnts, uint64_t conflicts, uint64_t literals);
    void printFinalStats(double cpu_time, double mem_used, uint64_t conflicts, uint64_t propagations);
    void printAllocatorStatistics();

#ifdef SOLVER_STATS
    inline void solverDecisionsInc() { ++decisions; }
    inline void solverRandomDecisionsInc() { ++rnd_decisions; }
    inline void solverDecisionVariablesInc() { ++dec_vars; }
    inline void solverDecisionVariablesDec() { --dec_vars; }
    inline void solverMaxLiteralsInc(int amount) { max_literals += amount; }
    inline void solverTotLiteralsInc(int amount) { tot_literals += amount; }
#else
    inline void solverDecisionsInc() { }
    inline void solverRandomDecisionsInc() { }
    inline void solverDecisionVariablesInc() { }
    inline void solverDecisionVariablesDec() { }
    inline void solverMaxLiteralsInc(int amount) { }
    inline void solverTotLiteralsInc(int amount) { }
#endif// SOLVER_STATS

#ifdef RESTART_STATS
    inline void solverRestartInc() { ++starts; }
    inline void solverStopsRestartsInc() { ++nbstopsrestarts; }
    inline void solverStopsRestartsSameInc() { ++nbstopsrestartssame; }
    inline void solverLastBlockAtRestartSave() { lastblockatrestart = starts; }
#else
    inline void solverRestartInc() { }
    inline void solverStopsRestartsInc() { }
    inline void solverStopsRestartsSameInc() { }
    inline void solverLastBlockAtRestartSave() { }
#endif// RESTART_STATS

#ifdef LEARNTS_STATS
    inline void solverRemovedClausesInc(int amount) { nbRemovedClauses += amount; }
    inline void solverReducedClausesInc() { ++nbReducedClauses; }
    inline void solverReduceDBInc() { ++nbReduceDB; }
    inline void solverUnariesInc() { ++nbUn; }
    inline void solverBinariesInc() { ++nbBin; }
    inline void solverLBD2Inc() { ++nbDL2; }
#else
    inline void solverRemovedClausesInc(int amount) { }
    inline void solverReducedClausesInc() { }
    inline void solverReduceDBInc() { }
    inline void solverUnariesInc() { }
    inline void solverBinariesInc() { }
    inline void solverLBD2Inc() { }
#endif// LEARNTS_STATS

#ifdef INCREMENTAL_STATS
    inline void incNBSatCalls() { ++nbSatCalls; }
    inline void incNBUnsatCalls() { ++nbUnsatCalls; }
    inline void incTotalTime4Sat(int amount) { totalTime4Sat += amount; }
    inline void incTotalTime4Unsat(int amount) { totalTime4Unsat += amount; }
#else
    inline void incNBSatCalls() { }
    inline void incNBUnsatCalls() { }
    inline void incTotalTime4Sat(int amount) { }
    inline void incTotalTime4Unsat(int amount) { }
#endif// INCREMENTAL_STATS

#ifdef ALLOCATOR_STATS
    inline void allocatorNumberOfPoolsSet(uint32_t nPools) {
        stats_number_of_pools = nPools;
        stats_pool_allocd.resize(nPools, 0);
        stats_pool_hwm.resize(nPools, 0);
    }
    inline void allocatorPoolAllocdInc(uint32_t index) {
        assert(index < stats_number_of_pools);
        ++stats_pool_allocd[index];
        stats_pool_hwm[index] = std::max(stats_pool_hwm[index], stats_pool_allocd[index]);
    }
    inline void allocatorXXLPoolAllocdInc() {
        ++stats_xxl_pool_allocd;
        stats_xxl_pool_hwm = std::max(stats_xxl_pool_hwm, stats_xxl_pool_allocd);
    }
    inline void allocatorBeyondMallocdInc() {
        ++stats_beyond_mallocd;
        stats_beyond_hwm = std::max(stats_beyond_hwm, stats_beyond_mallocd);
    }
    inline void allocatorPoolAllocdDec(uint32_t index) { assert(index < stats_number_of_pools); --stats_pool_allocd[index]; }
    inline void allocatorXXLPoolAllocdDec() { --stats_xxl_pool_allocd; }
    inline void allocatorBeyondMallocdDec() { --stats_beyond_mallocd; }
#else
    inline void allocatorNumberOfPoolsSet(uint32_t nPools) { }
    inline void allocatorPoolAllocdInc(uint32_t index) { }
    inline void allocatorXXLPoolAllocdInc() { }
    inline void allocatorBeyondMallocdInc() { }
    inline void allocatorPoolAllocdDec(uint32_t index) { }
    inline void allocatorXXLPoolAllocdDec() { }
    inline void allocatorBeyondMallocdDec() { }
#endif// ALLOCATOR_STATS

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_STATISTICS_H_ */
