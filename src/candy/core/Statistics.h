/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#ifndef SRC_CANDY_CORE_STATISTICS_H_
#define SRC_CANDY_CORE_STATISTICS_H_

#include <cstdint>
#include <cstdio>
#include <cassert>
#include <vector>
#include <map>
#include <string>

#include "candy/utils/Runtime.h"

#define SOLVER_STATS
#define RUNTIME_STATS

namespace Candy {

class Statistics {
#ifdef SOLVER_STATS
    uint64_t decisions;
    uint64_t starts;
    uint64_t nbReduceDB, nbRemovedClauses, nbReducedClauses;
    uint64_t subsumed, deleted;
#endif

#ifdef RUNTIME_STATS
    std::map<std::string, double> runtimes;
    std::map<std::string, double> starttimes;
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

    inline void solverRemovedClausesInc(unsigned int amount) { nbRemovedClauses += amount; }
    inline void solverReducedClausesInc(unsigned int amount) { nbReducedClauses += amount; }
    inline void solverReduceDBInc() { ++nbReduceDB; }

    inline void solverSubsumedInc() { ++subsumed; }
    inline void solverDeletedInc() { ++deleted; }
#else
    inline void solverDecisionsInc() { }
    inline void solverRandomDecisionsInc() { }

    inline void solverRestartInc() { }

    inline void solverRemovedClausesInc(unsigned int amount) { (void)(amount); }
    inline void solverReducedClausesInc(unsigned int amount) { (void)(amount); }
    inline void solverReduceDBInc() { }

    inline void solverSubsumedInc() { }
    inline void solverDeletedInc() { }
#endif// SOLVER_STATS

#ifdef RUNTIME_STATS
    inline void runtimeReset(std::string key) {
        if (!starttimes.count(key)) {
            starttimes.insert({{key, 0}});
        }
        if (!runtimes.count(key)) {
            runtimes.insert({{key, 0}});
        }
        starttimes[key] = 0;
        runtimes[key] = 0;
    }
    inline void runtimeStart(std::string key) {
        if (!starttimes.count(key)) {
            starttimes.insert({{key, 0}});
        }
        starttimes[key] = get_wall_time();
    }
    inline void runtimeStop(std::string key) {
        if (!starttimes.count(key)) {
            starttimes.insert({{key, 0}});
        }
        if (!runtimes.count(key)) {
            runtimes.insert({{key, 0}});
        }
        runtimes[key] += get_wall_time() - starttimes[key]; 
    }
    void printRuntime(std::string key) {
        printf("c Runtime %-14s: %12.2f s\n", key.c_str(), runtimes[key]);
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
