/*
 * SolverStatistics.cc
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#include <core/Statistics.h>
#include "candy/utils/System.h"

namespace Candy {

Statistics::Statistics() :
#ifdef SOLVER_STATS
    decisions(0), dec_vars(0), max_literals(0), tot_literals(0),
    starts(0), nbstopsrestarts(0), nbstopsrestartssame(0), lastblockatrestart(0),
    nbReduceDB(0), nbRemovedClauses(0), nbReducedClauses(0), nbDL2(0), nbBin(0), nbUn(0),
    subsumed(0), deleted(0),
#endif
#ifdef RUNTIME_STATS
    runtimes(), starttimes(),
#endif
#ifdef INCREMENTAL_STATS
    nbSatCalls(0),
    nbUnsatCalls(0),
#endif
#ifdef ALLOCATOR_STATS
    stats_number_of_pools(0), stats_pool_allocd(), stats_xxl_pool_allocd(0), stats_beyond_mallocd(0),
    stats_pool_hwm(), stats_xxl_pool_hwm(0), stats_beyond_hwm(0),
#endif
    stats(0)
{
    (void)stats;
}

Statistics::~Statistics() { }

void Statistics::printIncrementalStats(uint64_t conflicts, uint64_t propagations) {
    printf("c---------- Glucose Stats -------------------------\n");
#ifdef SOLVER_STATS
    printf("c restarts              : %" PRIu64"\n", starts);

    printf("c nb ReduceDB           : %" PRIu64"\n", nbReduceDB);
    printf("c nb removed Clauses    : %" PRIu64"\n", nbRemovedClauses);
    printf("c nb learnts DL2        : %" PRIu64"\n", nbDL2);
    printf("c nb learnts size 2     : %" PRIu64"\n", nbBin);
    printf("c nb learnts size 1     : %" PRIu64"\n", nbUn);

    printf("c conflicts             : %" PRIu64"\n", conflicts);
    printf("c decisions             : %" PRIu64"\n", decisions);
    printf("c propagations          : %" PRIu64"\n", propagations);
#endif
#ifdef INCREMENTAL_STATS
    printf("\nc SAT Calls             : %d\n", nbSatCalls);
    printf("c UNSAT Calls           : %d\n", nbUnsatCalls);

    printf("c--------------------------------------------------\n");
#endif
}


void Statistics::printIntermediateStats(int trail, int clauses, int learnts, uint64_t conflicts) {
#if defined SOLVER_STATS
    printf("c |%5llu %8llu %4llu |%9d %10d |%5llu %10d %7llu %6llu %5llu %10llu |\n",
            starts, nbstopsrestarts, (conflicts / starts),
            (int)dec_vars - trail, clauses,
            nbReduceDB, learnts, nbBin, nbUn, nbDL2, nbRemovedClauses);
#endif
}

void Statistics::printSimplificationStats() {
#if defined SOLVER_STATS
    printf("c simplification        : %" PRIu64" subsumed, %" PRIu64" deleted literals)\n", subsumed, deleted);
#endif
}

void Statistics::printFinalStats(uint64_t conflicts, uint64_t propagations) {
    std::chrono::milliseconds cpu_time_millis = Glucose::cpuTime();
    double cpu_time = static_cast<double>(cpu_time_millis.count())/1000.0f;
    printf("c ==============================================================================================\n");
#ifdef SOLVER_STATS
    printf("c restarts              : %" PRIu64" (%" PRIu64" conflicts in avg)\n", starts, (starts > 0 ? conflicts / starts : 0));
    printf("c blocked restarts      : %" PRIu64" (multiple: %" PRIu64") \n", nbstopsrestarts, nbstopsrestartssame);
    printf("c last block at restart : %" PRIu64"\n", lastblockatrestart);

    printf("c nb ReduceDB           : %" PRIu64"\n", nbReduceDB);
    printf("c nb removed Clauses    : %" PRIu64"\n", nbRemovedClauses);
    printf("c nb reduced Clauses    : %" PRIu64"\n", nbReducedClauses);
    printf("c nb learnts DL2        : %" PRIu64"\n", nbDL2);
    printf("c nb learnts size 2     : %" PRIu64"\n", nbBin);
    printf("c nb learnts size 1     : %" PRIu64"\n", nbUn);

    printf("c conflicts             : %-12" PRIu64"   (%.0f /sec)\n", conflicts, conflicts / cpu_time);
    printf("c decisions             : %-12" PRIu64"   (%.0f /sec)\n", decisions, decisions / cpu_time);
    printf("c propagations          : %-12" PRIu64"   (%.0f /sec)\n", propagations, propagations / cpu_time);
    printf("c conflict literals     : %-12" PRIu64"   (%4.2f %% deleted)\n", tot_literals, (max_literals - tot_literals) * 100 / (double) max_literals);
#endif// SOLVER_STATS
    double mem_used = 0; //memUsedPeak();
    if (mem_used != 0) {
        printf("Memory used           : %.2f MB\n", mem_used);
    }
    printf("c CPU time              : %g s\n\n", cpu_time);
}

void Statistics::printAllocatorStatistics() {
#ifdef ALLOCATOR_STATS
    printf("\n========= [Pools usage] =========\n");
    for (size_t i = 0; i < stats_number_of_pools; i++) {
        printf("%u;", stats_pool_allocd[i]);
    }
    printf("\n========= [Pools maximum] =========\n");
    for (size_t i = 0; i < stats_number_of_pools; i++) {
        printf("%zu;", stats_pool_hwm[i]);
    }
    printf("\n========= [Clauses in XXL Pool] =========\n");
    printf("%u\n", stats_xxl_pool_allocd);
    printf("\n========= [Clauses Beyond Pool] =========\n");
    printf("%u\n", stats_beyond_mallocd);
#endif
}

} /* namespace Candy */
