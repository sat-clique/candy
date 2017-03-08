/*
 * SolverStatistics.cc
 *
 *  Created on: Mar 7, 2017
 *      Author: markus
 */

#include <core/SolverStatistics.h>
#include "candy/utils/System.h"

namespace Candy {

SolverStatistics::SolverStatistics() :
    sumDecisionLevels(0), nbRemovedClauses(0), nbReducedClauses(0), nbDL2(0), nbBin(0), nbUn(0), nbReduceDB(0),
    solves(0), starts(0), decisions(0), rnd_decisions(0),
    nbstopsrestarts(0), nbstopsrestartssame(0), lastblockatrestart(0), dec_vars(0),
    clauses_literals(0), max_literals(0), tot_literals(0),
    totalTime4Sat(0.), totalTime4Unsat(0.), nbSatCalls(0), nbUnsatCalls(0)
{
}

SolverStatistics::~SolverStatistics() { }

void SolverStatistics::printIncrementalStats(uint64_t conflicts, uint64_t propagations) {
    printf("c---------- Glucose Stats -------------------------\n");
    printf("c restarts              : %" PRIu64"\n", starts);
    printf("c nb ReduceDB           : %" PRIu64"\n", nbReduceDB);
    printf("c nb removed Clauses    : %" PRIu64"\n", nbRemovedClauses);
    printf("c nb learnts DL2        : %" PRIu64"\n", nbDL2);
    printf("c nb learnts size 2     : %" PRIu64"\n", nbBin);
    printf("c nb learnts size 1     : %" PRIu64"\n", nbUn);

    printf("c conflicts             : %" PRIu64"\n", conflicts);
    printf("c decisions             : %" PRIu64"\n", decisions);
    printf("c propagations          : %" PRIu64"\n", propagations);

    printf("\nc SAT Calls             : %d in %g seconds\n", nbSatCalls, totalTime4Sat);
    printf("c UNSAT Calls           : %d in %g seconds\n", nbUnsatCalls, totalTime4Unsat);

    printf("c--------------------------------------------------\n");
}


void SolverStatistics::printIntermediateStats(int trail, int clauses, int learnts, uint64_t conflicts) {
    printf("c | %8d   %7d    %5d | %7d %8d %8d | %5d %8d   %6d %8d | %6.3f %% |\n", (int) starts, (int) nbstopsrestarts, (int) (conflicts / starts),
                            (int)dec_vars - trail, clauses, (int) clauses_literals, (int) nbReduceDB, learnts, (int) nbDL2, (int) nbRemovedClauses, -1.0);
}

void SolverStatistics::printFinalStats(double cpu_time, double mem_used, uint64_t conflicts, uint64_t propagations) {
    printf("c restarts              : %" PRIu64" (%" PRIu64" conflicts in avg)\n", starts, (starts > 0 ? conflicts / starts : 0));
    printf("c blocked restarts      : %" PRIu64" (multiple: %" PRIu64") \n", nbstopsrestarts, nbstopsrestartssame);
    printf("c last block at restart : %" PRIu64"\n", lastblockatrestart);
    printf("c nb ReduceDB           : %" PRIu64"\n", nbReduceDB);
    printf("c nb removed Clauses    : %" PRIu64"\n", nbRemovedClauses);
    printf("c nb learnts DL2        : %" PRIu64"\n", nbDL2);
    printf("c nb learnts size 2     : %" PRIu64"\n", nbBin);
    printf("c nb learnts size 1     : %" PRIu64"\n", nbUn);

    printf("c conflicts             : %-12" PRIu64"   (%.0f /sec)\n", conflicts, conflicts / cpu_time);
    printf("c decisions             : %-12" PRIu64"   (%4.2f %% random) (%.0f /sec)\n", decisions,
                    (float) rnd_decisions * 100 / (float) decisions, decisions / cpu_time);
    printf("c propagations          : %-12" PRIu64"   (%.0f /sec)\n", propagations, propagations / cpu_time);
    printf("c conflict literals     : %-12" PRIu64"   (%4.2f %% deleted)\n", tot_literals,
                    (max_literals - tot_literals) * 100 / (double) max_literals);
    printf("c nb reduced Clauses    : %" PRIu64"\n", nbReducedClauses);

    if (mem_used != 0)
        printf("Memory used           : %.2f MB\n", mem_used);
    printf("c CPU time              : %g s\n", cpu_time);
    printf("\n");
}

} /* namespace Candy */
