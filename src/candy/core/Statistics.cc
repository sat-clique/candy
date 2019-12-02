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

#include "candy/core/Statistics.h"
#include "candy/utils/Runtime.h"
#include "candy/core/CandySolverInterface.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/Trail.h"

namespace Candy {

Statistics::Statistics(CandySolverInterface& solver_) : solver(solver_), 
    restarts(), runtimes(), starttimes()
{ }

Statistics::~Statistics() { }

size_t Statistics::nClauses() const {
    return solver.getClauseDatabase().size();
}
size_t Statistics::nConflicts() const {
    return solver.getClauseDatabase().result.nConflicts;
}
size_t Statistics::nReduceCalls() const {
    return solver.getClauseDatabase().nReduceCalls;
}
size_t Statistics::nReduced() const {
    return solver.getClauseDatabase().nReduced;
}

size_t Statistics::nVars() const {
    return solver.getAssignment().vardata.size();
}
size_t Statistics::nPropagations() const {
    return solver.getAssignment().nPropagations;
}
size_t Statistics::nDecisions() const {
    return solver.getAssignment().nDecisions;
}

size_t Statistics::nRestarts() const {
    return restarts;
}

void Statistics::solverRestartInc() { ++restarts; }

void Statistics::runtimeReset(std::string key) {
    starttimes[key] = 0;
    runtimes[key] = 0;
}

void Statistics::runtimeStart(std::string key) {
    starttimes[key] = get_wall_time();
    if (!runtimes.count(key)) runtimes[key] = 0;
}

void Statistics::runtimeStop(std::string key) {
    if (!starttimes.count(key)) return;
    runtimes[key] += get_wall_time() - starttimes[key]; 
}

void Statistics::printRuntime(std::string key) {
    printf("c Runtime %-14s: %12.2f s\n", key.c_str(), runtimes[key]);
}

void Statistics::printRuntimes() {
    for (auto pair : runtimes) {
        printRuntime(pair.first);
    }
}

void Statistics::printIncrementalStats() {
    printf("c restarts              : %zu\n", restarts);

    printf("c nb ReduceDB           : %zu\n", nReduceCalls());
    printf("c nb removed Clauses    : %zu\n", nReduced());

    printf("c conflicts             : %zu\n", nConflicts());
    printf("c decisions             : %zu\n", nDecisions());
    printf("c propagations          : %zu\n", nPropagations());
}


void Statistics::printIntermediateStats() {
    printf("c | %5zu (%zu conflicts in avg) | %10zu %10zu %5zu |\n", restarts, (nConflicts() / restarts), nClauses(), nReduceCalls(), nReduced());
}

void Statistics::printFinalStats() {
    double cpu_time = get_cpu_time();
    printf("c =================================================================\n");
    printf("c restarts              : %zu (%zu conflicts in avg)\n", restarts, (restarts > 0 ? (nConflicts() / restarts) : 0));

    printf("c nb ReduceDB           : %zu\n", nReduceCalls());
    printf("c nb removed Clauses    : %zu\n", nReduced());

    printf("c conflicts             : %-12zu   (%.0f /sec)\n", nConflicts(), nConflicts() / runtimes["Wallclock"]);
    printf("c decisions             : %-12zu   (%.0f /sec)\n", nDecisions(), nDecisions() / runtimes["Wallclock"]);
    printf("c propagations          : %-12zu   (%.0f /sec)\n", nPropagations(), nPropagations() / runtimes["Wallclock"]);

    double mem_used = 0; //memUsedPeak();
    if (mem_used != 0) {
        printf("c Memory used           : %.2f MB\n", mem_used);
    }
    printf("c CPU time              : %g s\n\n", cpu_time);
}

} /* namespace Candy */
