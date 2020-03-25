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

#include "candy/core/CandySolverInterface.h"
#include "candy/utils/Runtime.h"

namespace Candy {

class Statistics {
    CandySolverInterface& solver;

    uint64_t restarts;
    std::map<std::string, double> runtimes;
    std::map<std::string, double> starttimes;

public:
    Statistics(CandySolverInterface& solver);
    ~Statistics();

    void printStats();

    size_t nVars() const;
    size_t nClauses() const;

    size_t nConflicts() const;
    size_t nPropagations() const;
    size_t nDecisions() const;

    size_t nRestarts() const;

    void solverRestartInc();

    void runtimeReset(std::string key);
    void runtimeStart(std::string key);
    void runtimeStop(std::string key);

    void printRuntime(std::string key);
    void printRuntimes();

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_STATISTICS_H_ */
