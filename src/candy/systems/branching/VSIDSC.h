/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology
VSIDSC -- Copyright(c) 2019 Norbert Blümle, KIT - Karlsruhe Institute of Technology

Candy sources are based on Glucose which is based on MiniSat (see former copyrights below). 
Permissions and copyrights of Candy are exactly the same as Glucose and Minisat (see below).


--------------- Former Glucose Copyrights

 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
                                CRIL - Univ. Artois, France
                                LRI  - Univ. Paris Sud, France (2009-2013)
                                Labri - Univ. Bordeaux, France

Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it 
is based on. (see below).


--------------- Original Minisat Copyrights

Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

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
*************************************************************************************************/

#ifndef SRC_CANDY_CORE_VSIDSC_H_
#define SRC_CANDY_CORE_VSIDSC_H_

#include <vector>

#include "candy/mtl/Heap.h"
#include "candy/core/Trail.h"
#include "candy/core/CNFProblem.h"
#include "candy/utils/CheckedCast.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/systems/branching/BranchingDiversificationInterface.h"

#include <iostream> // cout, clog, cerr, etc.
// not using printf because it isn't typesafe and a relict from c. cout is pretty much the same but more like a stream and c++
#include "candy/systems/branching/stats/VSIDSCStats.h"
#include "candy/systems/branching/VSIDS.h"
#include "candy/teexgraph/BDGraph.h"
#include "candy/teexgraph/CenGraph.h"
#include "candy/utils/Runtime.h"

namespace Candy
{

class VSIDSC : public BranchingDiversificationInterface
{

private:
        struct VarOrderLt
        {
                std::vector<double> &activity;
                bool operator()(Var x, Var y) const
                {
                        return activity[x] > activity[y];
                }
                VarOrderLt(std::vector<double> &act) : activity(act) {}
        };

        ClauseDatabase &clause_db;
        Trail &trail;
        Glucose::Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable activity.
        std::vector<double> activity;         // A heuristic measurement of the activity of a variable.
        std::vector<char> polarity;           // The preferred polarity of each variable.
        Stamp<uint32_t> stamp;
        double var_inc; // Amount to bump next variable with.
        double var_decay;
        const double max_var_decay;
        const bool glucose_style_extra_bump = false;
        const bool initial_polarity = true;
        const double initial_activity = 0.0;
        const string pfx = "c VSIDSC-";
        Graph graph;
        Scope scope = FULL;
        const int centralityFactor = 1;
        const bool use_centrality_bump = true;
        const bool use_LWCC = false;
        const double samplesize_initial = 1.0;
        double samplesize = 1.0;
        const double samplesize_decay = 1.0; // decay samplesize to x %
        vector<double> centrality;           // idx is graph-related node-id!
        int dbSize_lastRecalc = 0;
        const double dbSize_recalc_factor = 0.0;
        const bool debug = false;
        bool graph_init = false;
        int graph_nodes_init = -1;  // either in graph or in LWCC
        long graph_edges_init = -1; // undirected edges at first run. in graph or in LWCC
        int graph_nodes_max = -1;   // max # of nodes. either in graph or in LWCC
        long graph_edges_max = -1;  // max # of edges. either in graph or in LWCC
        double graph_avg_degree = 0;
        double graph_density = 0; // density := the number of edges / the maximum number of edges
        long graph_nWCC = -1;     // Weakly connected components count

protected:
        VSIDSCStats statistics;

public:
        VSIDSC(ClauseDatabase &_clause_db, Trail &_trail,
               double _var_decay = SolverOptions::opt_vsids_var_decay,
               double _max_var_decay = SolverOptions::opt_vsids_max_var_decay,
               bool _glucose_style_extra_bump = SolverOptions::opt_vsids_extra_bump,
               int _vsidsc_mult = SolverOptions::opt_vsidsc_mult,
               bool _vsidsc_bump = SolverOptions::opt_vsidsc_bump,
               bool _vsidsc_scope = SolverOptions::opt_vsidsc_scope,
               double _vsidsc_samplesize = SolverOptions::opt_vsidsc_samplesize,
               double _vsidsc_samplesize_decay = SolverOptions::opt_vsidsc_samplesize_decay,
               double _vsidsc_dbsize_recalc = SolverOptions::opt_vsidsc_dbsize_recalc,
               bool _vsidsc_debug = SolverOptions::opt_vsidsc_debug) : clause_db(_clause_db), trail(_trail), order_heap(VarOrderLt(activity)),
                                                                       activity(), polarity(), stamp(),
                                                                       var_inc(1), var_decay(_var_decay), max_var_decay(_max_var_decay),
                                                                       glucose_style_extra_bump(_glucose_style_extra_bump),
                                                                       centralityFactor(_vsidsc_mult),
                                                                       use_centrality_bump(_vsidsc_bump),
                                                                       use_LWCC(_vsidsc_scope),
                                                                       samplesize_initial(_vsidsc_samplesize),
                                                                       samplesize(_vsidsc_samplesize),
                                                                       samplesize_decay(_vsidsc_samplesize_decay),
                                                                       dbSize_recalc_factor(_vsidsc_dbsize_recalc),
                                                                       debug(_vsidsc_debug),
                                                                       statistics(*this)
        {
                if (use_LWCC)
                        scope = LWCC;
        }

        void clear()
        {
                activity.clear();
                polarity.clear();
                order_heap.clear();
        }

        void init(const CNFProblem &problem)
        {
                if (trail.nVars() > activity.size())
                {
                        activity.resize(trail.nVars(), initial_activity);
                        polarity.resize(trail.nVars(), initial_polarity);
                        stamp.grow(trail.nVars());
                        order_heap.grow(trail.nVars());
                }
                reset(); // let the solver do its magic first - then we add ours
                // we can skip centrality init as long process_reduce() will do it upon the very first reduction (which happens right away)
                // do_centrality();
        }

        void reset()
        {
                std::vector<int> vs;
                for (Var v = 0; v < (Var)trail.nVars(); v++)
                {
                        if (trail.isDecisionVar(v))
                        {
                                vs.push_back(v);
                        }
                }
                order_heap.build(vs);
        }

        /*
         * Branching Diversification Interface for HordeSAT integration
         */
        void setPolarity(Var v, bool sign) override
        {
                polarity[v] = sign;
        }

        Lit getLastDecision() override
        {
                return trail[(*trail.trail_lim.rbegin())];
        }

        void setActivity(Var v, double act)
        {
                activity[v] = act;
        }
        /* */

        // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
        inline void varDecayActivity()
        {
                var_inc *= (1 / var_decay);
        }

        /*
         * Increase a variable with the current 'bump' value.
         * We are using this to bump based on centrality value
         * THIS is the place to integrate specialized bumping strategies if we want to use different methods based on e.g. graph properties.
         * If we wanted to use different bumping this should be integrated here.
        */
        inline void varBumpActivity(Var v)
        {
                double bumpFactor = var_inc;
                if (use_centrality_bump)
                {
                        bumpFactor = var_inc * (1 + (centrality[graph.mapNode(v.id)] * centralityFactor));
                }
                varBumpActivity(v, bumpFactor);
        }

        inline void varBumpActivity(Var v, double inc)
        {
                if ((activity[v] += inc) > 1e100)
                {
                        varRescaleActivity();
                }
                if (order_heap.inHeap(v))
                {
                        order_heap.decrease(v); // update order-heap
                }
        }

        void varRescaleActivity()
        {
                for (size_t i = 0; i < activity.size(); ++i)
                {
                        activity[i] *= 1e-100;
                }
                var_inc *= 1e-100;
        }

        void process_conflict()
        {
                if (clause_db.result.nConflicts % 5000 == 0 && var_decay < max_var_decay)
                {
                        var_decay += 0.01;
                }

                stamp.clear();
                for (const Clause *clause : clause_db.result.involved_clauses)
                {
                        for (Lit lit : *clause)
                        {
                                Var v = lit.var();
                                if (!stamp[v])
                                {
                                        stamp.set(v);
                                        varBumpActivity(v);
                                }
                        }
                }

                if (glucose_style_extra_bump)
                {
                        for (auto it = trail.begin(trail.decisionLevel()); it < trail.end(); it++)
                        {
                                Var v = it->var();
                                if (!stamp[v] && trail.reason(v) != nullptr && trail.reason(v)->isLearnt() && trail.reason(v)->getLBD() < clause_db.result.lbd)
                                {
                                        varBumpActivity(v);
                                }
                        }
                }

                varDecayActivity();

                // UPDATEVARACTIVITY trick (see competition'09 Glucose companion paper)
                unsigned int backtrack_level = clause_db.result.backtrack_level;
                for (auto it = trail.begin(backtrack_level); it != trail.end(); it++)
                {
                        Var v = it->var();
                        polarity[v] = it->sign();
                        if (!order_heap.inHeap(v) && trail.isDecisionVar(v))
                                order_heap.insert(v);
                }
        }

        void process_reduce()
        {
                // make sure that centrality is calculated upon the very first call!
                if (!graph_init || (dbSize_recalc_factor >= 1 && clause_db.size() > dbSize_lastRecalc * dbSize_recalc_factor))
                {
                        do_centrality();
                        dbSize_lastRecalc = clause_db.size();
                }
        }

        // selects the next literal to branch on
        inline Lit pickBranchLit()
        {
                Var next = var_Undef;

                // Activity based decision:
                while (next == var_Undef || trail.value(next) != l_Undef || !trail.isDecisionVar(next))
                {
                        if (order_heap.empty())
                        {
                                next = var_Undef;
                                break;
                        }
                        else
                        {
                                next = order_heap.removeMin();
                        }
                }

                if (debug)
                        cout << pfx << "NextLit=" << next.id << endl;
                return next == var_Undef ? lit_Undef : Lit(next, polarity[next]);
        }

        bool using_lwcc()
        {
                return use_LWCC;
        }

        long get_nEdges()
        {
                return graph.selfEdges(scope) + ((graph.edges(scope) - graph.selfEdges(scope)) / 2);
        }

        long get_nEdges_init()
        {
                return graph_edges_init;
        }

        long get_nEdges_max()
        {
                return graph_edges_max;
        }

        long get_nNodes()
        {
                return graph.nodes(scope);
        }

        long get_nNodes_init()
        {
                return graph_nodes_init;
        }

        long get_nNodes_max()
        {
                return graph_nodes_max;
        }

        double get_avg_degree()
        {
                return max(0.0, graph_avg_degree);
        }

        double get_density()
        {
                return graph_density;
        }

        long get_nWCC()
        {
                return graph_nWCC;
        }

        Scope get_Scope()
        {
                return scope;
        }

        double get_samplesize_init()
        {
                return samplesize_initial;
        }

        double get_samplesize()
        {
                return samplesize;
        }

        double get_samplesize_decay()
        {
                return samplesize_decay;
        }

        double get_dbSize_recalcFactor()
        {
                return dbSize_recalc_factor;
        }

        VSIDSCStats &getStatistics()
        {
                return statistics;
        }

private:
        void do_centrality()
        {
                statistics.centralityCalculationsInc();
                statistics.runtimeStart("z Sum_ALL_CALCS");    // total sum of all centrality actions
                statistics.runtimeStartPerCalc("d Sum_cent_"); // cent calculation sum including graph, calc, updating activities, ...
                if (debug)
                        cout << pfx << ":Run=" << statistics.nCalculations() << ",Scope=" << scope << ",Samplesize=" << samplesize << ",Decay=" << samplesize_decay << ",Recalc=" << dbSize_recalc_factor << endl;

                statistics.printInfo("Centrality Run");
                statistics.runtimeStart("v Sum_Graph_only");
                statistics.runtimeStartPerCalc("a Graph_only_"); // time for building graph
                init_graph();                                    // reduce changes graph heavily so we need to re-init graph and recalculate centrality
                long edges = graph.selfEdges(scope) + ((graph.edges(scope) - graph.selfEdges(scope)) / 2);
                if (!graph_init)
                {
                        graph_nodes_init = graph.nodes(scope);
                        graph_edges_init = edges;
                        graph_avg_degree = graph.averageDegree(scope);
                        graph_density = graph.density(scope);
                        if (use_LWCC)
                        {
                                graph_nWCC = graph.wccCount();
                        }
                        graph_init = true;
                }
                graph_nodes_max = max(graph_nodes_max, graph.nodes(scope));
                graph_edges_max = max(graph_edges_max, edges);

                statistics.runtimeStopPerCalc("a Graph_only_");
                statistics.runtimeStop("v Sum_Graph_only");

                statistics.runtimeStart("x Sum_Calc_only");
                statistics.runtimeStartPerCalc("b Calc_only_"); // time just for centrality calculation
                calcCentrality();
                statistics.runtimeStopPerCalc("b Calc_only_");
                statistics.runtimeStop("x Sum_Calc_only");

                updateActivities();
                statistics.printInfo("Centrality End");
                samplesize *= samplesize_decay;
                statistics.runtimeStopPerCalc("d Sum_cent_");
                statistics.runtimeStop("z Sum_ALL_CALCS");
                statistics.printIntermediateStats("z Sum_ALL_CALCS");
        }

        // Initialize the graph for centrality calculations
        void init_graph()
        {
                graph = Graph(std::max(1u, clause_db.nVars())); // empty graph with max number of literals (trivial case with 0 vars triggers assertion in graph-library)
                for (const Clause *clause : clause_db)
                {
                        if (clause->isDeleted())
                                continue;
                        vector<int> literals;
                        for (Lit literal : *clause)
                        { // read all literals. We only care about the literal itself - not its polarity
                                int pure_literal = literal.var().id;
                                literals.push_back(pure_literal);
                        }
                        for (unsigned i = 0; i < literals.size() - 1; ++i)
                        {
                                for (unsigned j = i + 1; j < literals.size(); ++j)
                                {
                                        graph.addEdge(graph.mapNode(literals[i]), graph.mapNode(literals[j]));
                                }
                        }
                }
                // if (graph.nodes(scope) > 0 && graph.edges(scope) > 0)
                graph.makeUndirected(); //does remove redundant edges, too.
                if (use_LWCC)           // if we use LWCC-Mode we need to compute WCC
                {
                        graph.computeWCC();
                }
        }

        // Calculate Centrality and its distribution
        void calcCentrality()
        {
                // possible to have different centralities per parameter
                if (graph.nodes(scope) > 0 && graph.edges(scope) > 0)
                {
                        centrality = graph.betweennessCentrality(scope, samplesize);
                        if (debug)
                                printCentrality();
                }
        }

        // used to update all activities based on centrality at once
        void updateActivities()
        {
                if (!(graph.nodes(scope) > 0 && graph.edges(scope) > 0))
                        return;

                for (unsigned i = 0; i < trail.nVars(); ++i)
                {
                        // TODO: check why the result of mapNode exceeds centrality.size() in some cases
                        if ((size_t)graph.mapNode(i) < centrality.size()) {
                                double newActivity = centrality[graph.mapNode(i)] * centralityFactor;
                                setActivity(i, newActivity);
                        }
                }
                reset(); // we need to rebuild heap
                if (debug)
                        printActivities();
        }

        /* Calculates distribution of centrality
         * i=0 -> [0.0, 0.1[
         * i=1 -> [0.1, 0.2[
         * i=2 -> [0.2, 0.3[
         * ...
         * i=10 -> [1.0] (also maximum value centrality can be)
         * 
         * For debug purposes only
         */
        vector<vector<long>> calcCentralityDistribution()
        {
                vector<vector<long>> cenDistri(11, vector<long>());
                if (!debug)
                        return cenDistri;
                cout << pfx << "Cent: ";
                for (unsigned i = 0; i < centrality.size(); ++i)
                {
                        int distIdx = (int)(centrality[i] * 10);
                        cenDistri[distIdx].push_back(graph.revMapNode(i));
                        if (centrality[i] != 0)
                                cout << graph.revMapNode(i) << "=" << centrality[i] << ", ";
                }
                cout << endl;
                return cenDistri;
        }

        // for debug-purposes only!
        void printActivities()
        {
                if (!debug)
                        return;
                clog << pfx << "Act: ";
                for (unsigned i = 0; i < activity.size(); ++i)
                {
                        if (activity[i] != 0)
                                clog << i << "=" << activity[i] << ", ";
                }
                clog << endl;
        }

        // for debug purposes only
        void printCentrality()
        {
                if (!debug)
                        return;
                clog << pfx << "Graph: Nodes n = " << graph.nodes(scope) << ", Edges m = " << graph.edges(scope) << endl;
                vector<vector<long>> distribution = calcCentralityDistribution();
                for (unsigned i = 0; i < distribution.size(); ++i)
                {
                        if (distribution[i].size() > 0)
                        {
                                cout << pfx << "Distr(" << i << ") ";
                                for (long literal : distribution[i])
                                {
                                        cout << literal << ",";
                                }
                                cout << endl;
                        }
                }
        }
};

} // namespace Candy
#endif /* SRC_CANDY_CORE_VSIDSC_H_ */
