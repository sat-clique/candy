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
//#define RUNTIME_STATS
//#define INCREMENTAL_STATS
//#define ALLOCATOR_STATS

namespace Candy {

class Statistics {
#ifdef SOLVER_STATS
    uint64_t decisions, dec_vars, max_literals, tot_literals;
    uint64_t starts, nbstopsrestarts, nbstopsrestartssame, lastblockatrestart;
    uint64_t nbReduceDB, nbRemovedClauses, nbReducedClauses, nbDL2, nbBin, nbUn;
    uint64_t subsumed, deleted;
#endif// SOLVER_STATS

#ifdef RUNTIME_STATS
    std::map<std::string, std::chrono::milliseconds> runtimes;
    std::map<std::string, std::chrono::milliseconds> starttimes;
#endif// RUNTIME_STATS

#ifdef INCREMENTAL_STATS
    int nbSatCalls, nbUnsatCalls;
#endif// INCREMENTAL_STATS

#ifdef ALLOCATOR_STATS
    std::array<uint32_t, 502> stats_pool_allocs, stats_pool_deallocs;
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
    void printIntermediateStats(int trail, int clauses, int learnts, uint64_t conflicts);
    void printSimplificationStats();
    void printFinalStats(uint64_t conflicts, uint64_t propagations);
    void printAllocatorStatistics();

#ifdef SOLVER_STATS
    inline void solverDecisionsInc() { ++decisions; }
    inline void solverDecisionVariablesInc() { ++dec_vars; }
    inline void solverDecisionVariablesDec() { --dec_vars; }
    inline void solverMaxLiteralsInc(unsigned int amount) { max_literals += amount; }
    inline void solverTotLiteralsInc(unsigned int amount) { tot_literals += amount; }

    inline void solverRestartInc() { ++starts; }
    inline void solverStopsRestartsInc() { ++nbstopsrestarts; }
    inline void solverStopsRestartsSameInc() { ++nbstopsrestartssame; }
    inline void solverLastBlockAtRestartSave() { lastblockatrestart = starts; }

    inline void solverRemovedClausesInc(unsigned int amount) { nbRemovedClauses += amount; }
    inline void solverReducedClausesInc(unsigned int amount) { nbReducedClauses += amount; }
    inline void solverReduceDBInc() { ++nbReduceDB; }
    inline void solverUnariesInc() { ++nbUn; }
    inline void solverBinariesInc() { ++nbBin; }
    inline void solverLBD2Inc() { ++nbDL2; }

    inline void solverSubsumedInc() { ++subsumed; }
    inline void solverDeletedInc() { ++deleted; }
#else
    inline void solverDecisionsInc() { }
    inline void solverRandomDecisionsInc() { }
    inline void solverDecisionVariablesInc() { }
    inline void solverDecisionVariablesDec() { }
    inline void solverMaxLiteralsInc(unsigned int amount) { (void)(amount); }
    inline void solverTotLiteralsInc(unsigned int amount) { (void)(amount); }

    inline void solverRestartInc() { }
    inline void solverStopsRestartsInc() { }
    inline void solverStopsRestartsSameInc() { }
    inline void solverLastBlockAtRestartSave() { }

    inline void solverRemovedClausesInc(unsigned int amount) { (void)(amount); }
    inline void solverReducedClausesInc(unsigned int amount) { (void)(amount); }
    inline void solverReduceDBInc() { }
    inline void solverUnariesInc() { }
    inline void solverBinariesInc() { }
    inline void solverLBD2Inc() { }

    inline void solverSubsumedInc() { }
    inline void solverDeletedInc() { }
#endif// SOLVER_STATS


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
        printf("c Runtime %-14s: %12.2f s\n", key.c_str(), seconds);
    }
    void printRuntimes() {
        for (auto pair : runtimes) {
            printRuntime(pair.first);
        }
    }
#else
    inline void runtimeStart(std::string key) { (void)(key); }
    inline void runtimeStop(std::string key) { (void)(key); }
    void printRuntime(std::string key) { (void)(key); }
    void printRuntimes() { }
#endif// RUNTIME_STATS
#ifdef ALLOCATOR_STATS
    inline void allocatorPoolAlloc(uint32_t index) {
        assert(index < stats_number_of_pools);
        ++stats_pool_allocs[index];
    }
    inline void allocatorPoolDealloc(uint32_t index) {
        assert(index < stats_number_of_pools);
        ++stats_pool_deallocs[index];
    }
#else
    inline void allocatorPoolAlloc(uint32_t index) { (void)(index); }
    inline void allocatorPoolDealloc(uint32_t index) { (void)(index); }
#endif// ALLOCATOR_STATS

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_STATISTICS_H_ */
