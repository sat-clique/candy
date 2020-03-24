/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology
Statistics for VSIDSC -- Copyright(c) 2019 Norbert Blümle, KIT - Karlsruhe Institute of Technology

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

#ifndef SRC_CANDY_SYSTEMS_BRANCHING_VSIDSC_STATISTICS_H_
#define SRC_CANDY_SYSTEMS_BRANCHING_VSIDSC_STATISTICS_H_

#include <cstdint>
#include <cstdio>
#include <cassert>
#include <vector>
#include <map>
#include <string>

namespace Candy
{
class VSIDSC; // forward declaration

class VSIDSCStats
// class VSIDSCStats
{
private:
    VSIDSC &brancher;
    std::map<std::string, double> runtimes;
    std::map<std::string, double> starttimes;
    uint32_t calculations = 0;

public:
    VSIDSCStats(VSIDSC &brancher);
    ~VSIDSCStats();

private:
    // Number of initial nodes in graph
    int nNodes_init() const;

    // Number of max nodes in graph
    int nNodes_max() const;

    // Number of current nodes in graph
    int nNodes() const;

    // Number of initial edges in graph
    long nEdges_init() const;

    // Number of max edges in graph
    long nEdges_max() const;

    // Number of current edges in graph
    long nEdges() const;

    // average degree in scope
    double avg_degree() const;

    // density := the number of edges / the maximum number of edges 
    double density() const;

    // Number of WCCs in graph
    long nWCC() const;

    std::string scope() const;

    size_t samplesize() const;
    size_t samplesize_last() const;
    size_t samplesize_decay() const;

    size_t dbSizeRecalc() const;

public:
    size_t nCalculations() const;

    void centralityCalculationsInc();

    void runtimeStartPerCalc(std::string key);
    void runtimeStopPerCalc(std::string key);

    void printInfo(std::string text);

    void printIncrementalStats();
    void printIntermediateStats(std::string runtime_calc);
    void printFinalStats();

    void runtimeReset(std::string key);
    void runtimeStart(std::string key);
    void runtimeStop(std::string key);
    void printRuntime(std::string key);
    void printRuntimes();
};

} /* namespace Candy */

#endif /* SRC_CANDY_SYSTEMS_BRANCHING_VSIDSC_STATISTICS_H_ */
