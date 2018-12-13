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

#define SOLVER_STATS
//#define RUNTIME_STATS

namespace Candy {

class Statistics {
#ifdef SOLVER_STATS
    uint64_t decisions;
    uint64_t starts, nbstopsrestarts, nbstopsrestartssame, lastblockatrestart;
    uint64_t nbReduceDB, nbRemovedClauses, nbReducedClauses;
    uint64_t subsumed, deleted;
#endif

#ifdef RUNTIME_STATS
    std::map<std::string, std::chrono::milliseconds> runtimes;
    std::map<std::string, std::chrono::milliseconds> starttimes;
#endif

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

    inline void solverRestartInc() { ++starts; }
    inline void solverStopsRestartsInc() { ++nbstopsrestarts; }
    inline void solverStopsRestartsSameInc() { ++nbstopsrestartssame; }
    inline void solverLastBlockAtRestartSave() { lastblockatrestart = starts; }

    inline void solverRemovedClausesInc(unsigned int amount) { nbRemovedClauses += amount; }
    inline void solverReducedClausesInc(unsigned int amount) { nbReducedClauses += amount; }
    inline void solverReduceDBInc() { ++nbReduceDB; }

    inline void solverSubsumedInc() { ++subsumed; }
    inline void solverDeletedInc() { ++deleted; }
#else
    inline void solverDecisionsInc() { }
    inline void solverRandomDecisionsInc() { }

    inline void solverRestartInc() { }
    inline void solverStopsRestartsInc() { }
    inline void solverStopsRestartsSameInc() { }
    inline void solverLastBlockAtRestartSave() { }

    inline void solverRemovedClausesInc(unsigned int amount) { (void)(amount); }
    inline void solverReducedClausesInc(unsigned int amount) { (void)(amount); }
    inline void solverReduceDBInc() { }

    inline void solverSubsumedInc() { }
    inline void solverDeletedInc() { }
#endif// SOLVER_STATS

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
        starttimes[key] = cpuTime();
    }
    inline void runtimeStop(std::string key) {
        if (!starttimes.count(key)) {
            starttimes.insert({{key, std::chrono::milliseconds{0}}});
        }
        if (!runtimes.count(key)) {
            runtimes.insert({{key, std::chrono::milliseconds{0}}});
        }
        runtimes[key] += cpuTime() - starttimes[key];
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

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_STATISTICS_H_ */
