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

#ifndef SRC_CANDY_CORE_LOGGING_H_
#define SRC_CANDY_CORE_LOGGING_H_

#include "candy/core/CandySolverInterface.h"
#include "candy/core/clauses/Clause.h"

#include "candy/frontend/CLIOptions.h"
#include "candy/sonification/SolverSonification.h"

namespace Candy {

class Logging {
    CandySolverInterface& solver;

    SolverSonification sonification;
    int sonification_delay;

public:
    Logging(CandySolverInterface& solver_)
     : solver(solver_), sonification(), sonification_delay(SolverOptions::opt_sonification_delay) { }
    ~Logging() { }

    inline void logStart() {
        sonification.start(solver.getStatistics().nVars(), solver.getStatistics().nClauses());
    }

    inline void logRestart() {
        sonification.restart();
    }

    inline void logDecision() {
        sonification.decisionLevel(solver.getAssignment().decisionLevel(), sonification_delay);
        sonification.assignmentLevel(solver.getAssignment().size());
    }

    inline void logConflict() {
        sonification.conflictLevel(solver.getAssignment().size());
    }

    inline void logLearntClause(Clause* clause) {
        sonification.learntSize(clause->size());
    }

    inline void logResult(lbool status) {
        if (status == l_False) {
            sonification.stop(1);
        }
        else if (status == l_True) {
            sonification.stop(0);
        }
        else {
            sonification.stop(-1);
        }
    }

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_STATISTICS_H_ */
