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
#endif
#ifdef RESTART_STATS
    starts(0), nbstopsrestarts(0), nbstopsrestartssame(0), lastblockatrestart(0),
#endif
#ifdef LEARNTS_STATS
    nbReduceDB(0), nbRemovedClauses(0), nbReducedClauses(0), nbDL2(0), nbBin(0), nbUn(0),
#endif
#ifdef RUNTIME_STATS
    runtimes({{RT_INITIALIZATION, 0}, {RT_SOLVER, 0}, {RT_GATOR, 0}, {RT_SIMPLIFIER, 0}}),
    starttimes({{RT_INITIALIZATION, 0}, {RT_SOLVER, 0}, {RT_GATOR, 0}, {RT_SIMPLIFIER, 0}}),
#endif
#ifdef INCREMENTAL_STATS
    totalTime4Sat(0.), totalTime4Unsat(0.), nbSatCalls(0), nbUnsatCalls(0),
#endif
#ifdef ALLOCATOR_STATS
    stats_number_of_pools(0), stats_pool_allocd(), stats_xxl_pool_allocd(0), stats_beyond_mallocd(0),
    stats_pool_hwm(), stats_xxl_pool_hwm(0), stats_beyond_hwm(0),
#endif
    stats(0)
{
}

Statistics::~Statistics() { }

void Statistics::printIncrementalStats(uint64_t conflicts, uint64_t propagations) {
    printf("c---------- Glucose Stats -------------------------\n");
#ifdef RESTART_STATS
    printf("c restarts              : %" PRIu64"\n", starts);
#endif
#ifdef LEARNTS_STATS
    printf("c nb ReduceDB           : %" PRIu64"\n", nbReduceDB);
    printf("c nb removed Clauses    : %" PRIu64"\n", nbRemovedClauses);
    printf("c nb learnts DL2        : %" PRIu64"\n", nbDL2);
    printf("c nb learnts size 2     : %" PRIu64"\n", nbBin);
    printf("c nb learnts size 1     : %" PRIu64"\n", nbUn);
#endif
#ifdef SOLVER_STATS
    printf("c conflicts             : %" PRIu64"\n", conflicts);
    printf("c decisions             : %" PRIu64"\n", decisions);
    printf("c propagations          : %" PRIu64"\n", propagations);
#endif
#ifdef INCREMENTAL_STATS
    printf("\nc SAT Calls             : %d in %g seconds\n", nbSatCalls, totalTime4Sat);
    printf("c UNSAT Calls           : %d in %g seconds\n", nbUnsatCalls, totalTime4Unsat);

    printf("c--------------------------------------------------\n");
#endif
}


void Statistics::printIntermediateStats(int trail, int clauses, int learnts, uint64_t conflicts, uint64_t literals) {
#if defined SOLVER_STATS && defined RESTART_STATS && defined LEARNTS_STATS
    printf("c | %8llu   %7llu    %5llu | %7d %8d %8llu | %5llu %8d   %6llu %8llu |\n",
            starts, nbstopsrestarts, (conflicts / starts),
            (int)dec_vars - trail, clauses, literals,
            nbReduceDB, learnts, nbDL2, nbRemovedClauses);
#endif
}

void Statistics::printFinalStats(uint64_t conflicts, uint64_t propagations) {
    double cpu_time = Glucose::cpuTime();
    printf("c ==============================================================================================\n");
#ifdef RESTART_STATS
    printf("c restarts              : %" PRIu64" (%" PRIu64" conflicts in avg)\n", starts, (starts > 0 ? conflicts / starts : 0));
    printf("c blocked restarts      : %" PRIu64" (multiple: %" PRIu64") \n", nbstopsrestarts, nbstopsrestartssame);
    printf("c last block at restart : %" PRIu64"\n", lastblockatrestart);
#endif
#ifdef LEARNTS_STATS
    printf("c nb ReduceDB           : %" PRIu64"\n", nbReduceDB);
    printf("c nb removed Clauses    : %" PRIu64"\n", nbRemovedClauses);
    printf("c nb reduced Clauses    : %" PRIu64"\n", nbReducedClauses);
    printf("c nb learnts DL2        : %" PRIu64"\n", nbDL2);
    printf("c nb learnts size 2     : %" PRIu64"\n", nbBin);
    printf("c nb learnts size 1     : %" PRIu64"\n", nbUn);
#endif
#ifdef SOLVER_STATS
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
