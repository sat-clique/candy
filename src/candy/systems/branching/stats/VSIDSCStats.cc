/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology
VSIDSCStats -- Copyright(c) 2019 Norbert Blümle, KIT - Karlsruhe Institute of Technology
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

#include "candy/systems/branching/stats/VSIDSCStats.h"
#include "candy/systems/branching/VSIDSC.h"
#include "candy/utils/Runtime.h"
#include "candy/teexgraph/Graph.h"

// Keeping printf here to be more in-line with other stats modules.
// Might be a good idea to create an overall stats interface to be used by each branching tool

namespace Candy
{

VSIDSCStats::VSIDSCStats(VSIDSC &brancher_) : brancher(brancher_), runtimes(), starttimes()
{
}

VSIDSCStats::~VSIDSCStats() {}

// Number of initial nodes in graph
int VSIDSCStats::nNodes_init() const
{
    return brancher.get_nNodes_init();
}

// Number of max nodes in graph
int VSIDSCStats::nNodes_max() const
{
    return brancher.get_nNodes_max();
}

// Number of current nodes in graph
int VSIDSCStats::nNodes() const
{
    return brancher.get_nNodes();
}

// Number of initial edges in graph
long VSIDSCStats::nEdges_init() const
{
    return brancher.get_nEdges_init();
}

// Number of max edges in graph
long VSIDSCStats::nEdges_max() const
{
    return brancher.get_nEdges_max();
}

// Number of current edges in graph
long VSIDSCStats::nEdges() const
{
    return brancher.get_nEdges();
}

double VSIDSCStats::avg_degree() const
{
    return brancher.get_avg_degree();
}

double VSIDSCStats::density() const
{
    return brancher.get_density();
}

long VSIDSCStats::nWCC() const
{
    return brancher.get_nWCC();
}

std::string VSIDSCStats::scope() const
{
    switch (brancher.get_Scope())
    {
    case Scope::FULL:
        return "FULL";
        break;
    case Scope::LSCC:
        return "LSCC";
        break;
    case Scope::LWCC:
        return "LWCC";
        break;
    default:
        return "WRONG SCOPE";
    }
}

size_t VSIDSCStats::samplesize() const
{
    return brancher.get_samplesize_init() * 100;
}

size_t VSIDSCStats::samplesize_last() const
{
    return brancher.get_samplesize() * 100;
}

size_t VSIDSCStats::samplesize_decay() const
{
    return brancher.get_samplesize_decay() * 100;
}

size_t VSIDSCStats::dbSizeRecalc() const
{
    return brancher.get_dbSize_recalcFactor();
}

size_t VSIDSCStats::nCalculations() const
{
    return calculations;
}

void VSIDSCStats::centralityCalculationsInc() { ++calculations; }

void VSIDSCStats::runtimeStartPerCalc(std::string key)
{
    key += std::to_string(nCalculations());
    runtimeStart(key);
}

void VSIDSCStats::runtimeStopPerCalc(std::string key)
{
    key += std::to_string(nCalculations());
    runtimeStop(key);
}

void VSIDSCStats::runtimeReset(std::string key)
{
    starttimes[key] = 0;
    runtimes[key] = 0;
}

void VSIDSCStats::runtimeStart(std::string key)
{
    starttimes[key] = get_wall_time();
    if (!runtimes.count(key))
        runtimes[key] = 0;
}

void VSIDSCStats::runtimeStop(std::string key)
{
    if (!starttimes.count(key))
        return;
    runtimes[key] += get_wall_time() - starttimes[key];
}

void VSIDSCStats::printRuntime(std::string key)
{
    printf("c Runtime %-17s: %12.2f s\n", key.c_str(), runtimes[key]);
}

void VSIDSCStats::printRuntimes()
{
    for (auto pair : runtimes)
    {
        printRuntime(pair.first);
    }
}

void VSIDSCStats::printInfo(std::string text)
{
    printf("c | %-17s: %12.2f \n", text.c_str(), get_wall_time());
}

void VSIDSCStats::printIncrementalStats()
{
}

void VSIDSCStats::printIntermediateStats(std::string runtime_calc)
{
    printf("c | nCalc=%zu | curSample%%=%zu | Nodes=%d | Edges=%ld | avgDeg=%.3lf | Dens=%.3lf | ", nCalculations(), samplesize_last(), nNodes(), nEdges(), avg_degree(), density());
    printRuntime(runtime_calc);
}

void VSIDSCStats::printFinalStats()
{
    // printf("c =================================================================\n");
    printf("c --------------------- VSIDSC Statistics -------------------------\n");
    printf("c | Samplesize%%=%zu->%zu | SampleDecay%%=%zu | DB-SizeRecalcs=%zu |\n", samplesize(), samplesize_last(), samplesize_decay(), dbSizeRecalc());
    printf("c Graph Scope              : %s\n", scope().c_str());
    printf("c Centrality nCalc         : %zu\n", nCalculations());

    // Nodes / Edges are always based on the scope. So either the FULL graph or LWCC
    printf("c Graph Nodes init         : %d\n", nNodes_init());
    printf("c Graph Nodes max          : %d\n", nNodes_max());
    printf("c Graph Edges init         : %ld\n", nEdges_init());
    printf("c Graph Edges max          : %ld\n", nEdges_max());
    printf("c Graph average degree     : %.3lf\n", avg_degree());
    printf("c Graph density            : %.3lf\n", density());
    if (brancher.using_lwcc())
    {
        printf("c Graph LWCC count         : %lu\n", nWCC());
    }
    printRuntimes();
}

} /* namespace Candy */
