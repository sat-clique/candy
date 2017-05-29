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
#include <map>
#include <string>
#include <chrono>
#include <candy/utils/System.h>

//#define SOLVER_STATS
//#define RESTART_STATS
//#define LEARNTS_STATS
#define RUNTIME_STATS
//#define INCREMENTAL_STATS
//#define ALLOCATOR_STATS

namespace Candy {

class Statistics {
#ifdef SOLVER_STATS
    uint64_t decisions, dec_vars, max_literals, tot_literals;
#endif// SOLVER_STATS

#ifdef RESTART_STATS
    uint64_t starts, nbstopsrestarts, nbstopsrestartssame, lastblockatrestart;
#endif// RESTART_STATS

#ifdef LEARNTS_STATS
    uint64_t nbReduceDB, nbRemovedClauses, nbReducedClauses, nbDL2, nbBin, nbUn;
#endif// LEARNTS_STATS

#ifdef RUNTIME_STATS
#define RT_INITIALIZATION   "Runtime Initialization"
#define RT_SOLVER           "Runtime Solver"
#define RT_GATOR            "Runtime Gate Analyzer"
#define RT_SIMPLIFIER       "Runtime Simplifier"
    std::map<std::string, std::chrono::milliseconds> runtimes;
    std::map<std::string, std::chrono::milliseconds> starttimes;
#endif// RUNTIME_STATS

#ifdef INCREMENTAL_STATS
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
    void printFinalStats(uint64_t conflicts, uint64_t propagations);
    void printAllocatorStatistics();

#ifdef SOLVER_STATS
    inline void solverDecisionsInc() { ++decisions; }
    inline void solverDecisionVariablesInc() { ++dec_vars; }
    inline void solverDecisionVariablesDec() { --dec_vars; }
    inline void solverMaxLiteralsInc(unsigned int amount) { max_literals += amount; }
    inline void solverTotLiteralsInc(unsigned int amount) { tot_literals += amount; }
#else
    inline void solverDecisionsInc() { }
    inline void solverRandomDecisionsInc() { }
    inline void solverDecisionVariablesInc() { }
    inline void solverDecisionVariablesDec() { }
    inline void solverMaxLiteralsInc(unsigned int amount) { (void)(amount); }
    inline void solverTotLiteralsInc(unsigned int amount) { (void)(amount); }
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
    inline void solverRemovedClausesInc(unsigned int amount) { nbRemovedClauses += amount; }
    inline void solverReducedClausesInc(unsigned int amount) { nbReducedClauses += amount; }
    inline void solverReduceDBInc() { ++nbReduceDB; }
    inline void solverUnariesInc() { ++nbUn; }
    inline void solverBinariesInc() { ++nbBin; }
    inline void solverLBD2Inc() { ++nbDL2; }
#else
    inline void solverRemovedClausesInc(unsigned int amount) { (void)(amount); }
    inline void solverReducedClausesInc(unsigned int amount) { (void)(amount); }
    inline void solverReduceDBInc() { }
    inline void solverUnariesInc() { }
    inline void solverBinariesInc() { }
    inline void solverLBD2Inc() { }
#endif// LEARNTS_STATS

#ifdef INCREMENTAL_STATS
    inline void incNBSatCalls() { ++nbSatCalls; }
    inline void incNBUnsatCalls() { ++nbUnsatCalls; }
#else
    inline void incNBSatCalls() { }
    inline void incNBUnsatCalls() { }
#endif// INCREMENTAL_STATS
#ifdef RUNTIME_STATS
    inline void runtimeReset(std::string key) {
        if (!starttimes.count(key)) {
            starttimes.insert({{key, std::chrono::milliseconds{0}}});
        }
        if (!runtimes.count(key)) {
            runtimes.insert({{key, std::chrono::milliseconds{0}}});
        }
        starttimes[key] = std::chrono::milliseconds{0};
        runtimes[key] = std::chrono::milliseconds{0};
    }
    inline void runtimeStart(std::string key) {
        if (!starttimes.count(key)) {
            starttimes.insert({{key, std::chrono::milliseconds{0}}});
        }
        starttimes[key] = Glucose::cpuTime();
    }
    inline void runtimeStop(std::string key) {
        if (!starttimes.count(key)) {
            starttimes.insert({{key, std::chrono::milliseconds{0}}});
        }
        if (!runtimes.count(key)) {
            runtimes.insert({{key, std::chrono::milliseconds{0}}});
        }
        runtimes[key] += Glucose::cpuTime() - starttimes[key];
    }
    void printRuntime(std::string key) {
        double seconds = static_cast<double>(runtimes[key].count())/1000.0f;
        printf("c |  %-23s:  %12.2f s                                                  |\n", key.c_str(), seconds);
    }
#else
    inline void runtimeStart(std::string key) { (void)(key); }
    inline void runtimeStop(std::string key) { (void)(key); }
    void printRuntime(std::string key) { (void)(key); }
#endif// RUNTIME_STATS
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

    // keep statistics intact (although clause now leaks one literal)
    inline void allocatorStrengthenClause(uint32_t length) {
        uint16_t index = length-1;
        if (index < NUMBER_OF_POOLS) {
            allocatorPoolAllocdDec(index);
            allocatorPoolAllocdInc(index-1);
        }
        else if (index < XXL_POOL_ONE_SIZE && index-1 < NUMBER_OF_POOLS) {
            allocatorXXLPoolAllocdDec();
            allocatorPoolAllocdInc(index-1);
        }
        else if (index-1 < XXL_POOL_ONE_SIZE) {
            allocatorBeyondMallocdDec();
            allocatorXXLPoolAllocdInc();
        }
    }
#else
    inline void allocatorNumberOfPoolsSet(uint32_t nPools) { (void)(nPools); }
    inline void allocatorPoolAllocdInc(uint32_t index) { (void)(index); }
    inline void allocatorXXLPoolAllocdInc() { }
    inline void allocatorBeyondMallocdInc() { }
    inline void allocatorPoolAllocdDec(uint32_t index) { (void)(index); }
    inline void allocatorXXLPoolAllocdDec() { }
    inline void allocatorBeyondMallocdDec() { }
    inline void allocatorStrengthenClause(uint32_t length) { (void)(length); }
#endif// ALLOCATOR_STATS

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_STATISTICS_H_ */
