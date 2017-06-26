/*
 * SolverStatistics.cc
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#include <candy/core/Statistics.h>
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
    stats_pool_allocs(0), stats_pool_deallocs(0),
#endif
    stats(0)
{
    (void)stats;
}

Statistics::~Statistics() { }

void Statistics::printIncrementalStats(uint64_t conflicts, uint64_t propagations) {
    printf("c---------- Glucose Stats -------------------------\n");
#ifdef SOLVER_STATS
    printf("c restarts              : %llu\n", starts);

    printf("c nb ReduceDB           : %llu\n", nbReduceDB);
    printf("c nb removed Clauses    : %llu\n", nbRemovedClauses);
    printf("c nb learnts DL2        : %llu\n", nbDL2);
    printf("c nb learnts size 2     : %llu\n", nbBin);
    printf("c nb learnts size 1     : %llu\n", nbUn);

    printf("c conflicts             : %llu\n", conflicts);
    printf("c decisions             : %llu\n", decisions);
    printf("c propagations          : %llu\n", propagations);
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
    printf("c simplification        : %llu subsumed, %llu deleted literals)\n", subsumed, deleted);
#endif
}

void Statistics::printFinalStats(uint64_t conflicts, uint64_t propagations) {
    std::chrono::milliseconds cpu_time_millis = cpuTime();
    double cpu_time = static_cast<double>(cpu_time_millis.count())/1000.0f;
    printf("c ==============================================================================================\n");
#ifdef SOLVER_STATS
    printf("c restarts              : %llu (%llu conflicts in avg)\n", starts, (starts > 0 ? conflicts / starts : 0));
    printf("c blocked restarts      : %llu (multiple: %llu)\n", nbstopsrestarts, nbstopsrestartssame);
    printf("c last block at restart : %llu\n", lastblockatrestart);

    printf("c nb ReduceDB           : %llu\n", nbReduceDB);
    printf("c nb removed Clauses    : %llu\n", nbRemovedClauses);
    printf("c nb reduced Clauses    : %llu\n", nbReducedClauses);
    printf("c nb learnts DL2        : %llu\n", nbDL2);
    printf("c nb learnts size 2     : %llu\n", nbBin);
    printf("c nb learnts size 1     : %llu\n", nbUn);

    printf("c conflicts             : %-12llu   (%.0f /sec)\n", conflicts, conflicts / cpu_time);
    printf("c decisions             : %-12llu   (%.0f /sec)\n", decisions, decisions / cpu_time);
    printf("c propagations          : %-12llu   (%.0f /sec)\n", propagations, propagations / cpu_time);
    printf("c conflict literals     : %-12llu   (%4.2f %% deleted)\n", tot_literals, (max_literals - tot_literals) * 100 / (double) max_literals);
#endif// SOLVER_STATS
    double mem_used = 0; //memUsedPeak();
    if (mem_used != 0) {
        printf("Memory used           : %.2f MB\n", mem_used);
    }
    printf("c CPU time              : %g s\n\n", cpu_time);
}

void Statistics::printAllocatorStatistics() {
#ifdef ALLOCATOR_STATS
    printf("\n========= [Pool Usage] =========\n");
    for (size_t i = 0; i < stats_pool_allocs.size(); i++) {
        printf("Allocs %u; Deallocs %u; Used %u\n", stats_pool_allocs[i], stats_pool_deallocs[i], stats_pool_allocs[i] - stats_pool_deallocs[i]);
    }
#endif
}

} /* namespace Candy */
