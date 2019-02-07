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

#include <candy/core/Statistics.h>
#include <candy/utils/Runtime.h>

namespace Candy {

Statistics::Statistics() :
#ifdef SOLVER_STATS
    decisions(0), 
    starts(0), nbstopsrestarts(0), nbstopsrestartssame(0), lastblockatrestart(0),
    nbReduceDB(0), nbRemovedClauses(0), nbReducedClauses(0),
    subsumed(0), deleted(0),
#endif
#ifdef RUNTIME_STATS
    runtimes(), starttimes(),
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

    printf("c conflicts             : %llu\n", conflicts);
    printf("c decisions             : %llu\n", decisions);
    printf("c propagations          : %llu\n", propagations);
#endif
}


void Statistics::printIntermediateStats(int trail, int clauses, int learnts, uint64_t conflicts) {
#if defined SOLVER_STATS
    printf("c |%5llu %8llu %4llu | %11d |%5llu %10d %10llu |\n",
            starts, nbstopsrestarts, (conflicts / starts), clauses,
            nbReduceDB, learnts, nbRemovedClauses);
#endif
}

void Statistics::printSimplificationStats() {
#if defined SOLVER_STATS
    printf("c simplification        : %llu subsumed, %llu deleted literals)\n", subsumed, deleted);
#endif
}

void Statistics::printFinalStats(uint64_t conflicts, uint64_t propagations) {
    double cpu_time = get_cpu_time();
    printf("c =================================================================\n");
#ifdef SOLVER_STATS
    printf("c restarts              : %llu (%llu conflicts in avg)\n", starts, (starts > 0 ? conflicts / starts : 0));
    printf("c blocked restarts      : %llu (multiple: %llu)\n", nbstopsrestarts, nbstopsrestartssame);
    printf("c last block at restart : %llu\n", lastblockatrestart);

    printf("c nb ReduceDB           : %llu\n", nbReduceDB);
    printf("c nb removed Clauses    : %llu\n", nbRemovedClauses);
    printf("c nb reduced Clauses    : %llu\n", nbReducedClauses);

    printf("c conflicts             : %-12llu   (%.0f /sec)\n", conflicts, conflicts / cpu_time);
    printf("c decisions             : %-12llu   (%.0f /sec)\n", decisions, decisions / cpu_time);
    printf("c propagations          : %-12llu   (%.0f /sec)\n", propagations, propagations / cpu_time);
#endif
    double mem_used = 0; //memUsedPeak();
    if (mem_used != 0) {
        printf("c Memory used           : %.2f MB\n", mem_used);
    }
    printf("c CPU time              : %g s\n\n", cpu_time);
}

} /* namespace Candy */
