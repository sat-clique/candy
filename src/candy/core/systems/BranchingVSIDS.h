/*************************************************************************************************
Candy -- Copyright (c) 2015-2019, Markus Iser, KIT - Karlsruhe Institute of Technology

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

#ifndef SRC_CANDY_CORE_VSIDS_H_
#define SRC_CANDY_CORE_VSIDS_H_

#include <vector>

#include "candy/mtl/Heap.h"
#include "candy/core/Trail.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/systems/BranchingInterface.h"

namespace Candy {

class BranchingVSIDS : public BranchingInterface {
protected:
    ClauseDatabase &clause_db;
    Trail &trail;

public:
    struct VarOrderLt {
        std::vector<double> &activity;
        bool operator()(Var x, Var y) const
        {
            return activity[x] > activity[y];
        }
        VarOrderLt(std::vector<double> &act) : activity(act) {}
    };

    Glucose::Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable activity.
    std::vector<double> activity;         // A heuristic measurement of the activity of a variable.
    std::vector<char> polarity;           // The preferred polarity of each variable.
    Stamp<uint32_t> stamp;
    double var_inc; // Amount to bump next variable with.
    double var_decay;
    const double max_var_decay;
    const bool initial_polarity = true;
    const double initial_activity = 0.0;

    BranchingVSIDS(ClauseDatabase &_clause_db, Trail &_trail) : 
        clause_db(_clause_db), trail(_trail),
        order_heap(VarOrderLt(activity)),
        activity(), polarity(), stamp(),
        var_inc(1), 
        var_decay(SolverOptions::opt_vsids_var_decay), 
        max_var_decay(SolverOptions::opt_vsids_max_var_decay) 
    {
        activity.resize(clause_db.nVars(), initial_activity);
        polarity.resize(clause_db.nVars(), initial_polarity);
        stamp.grow(clause_db.nVars());
        order_heap.grow(clause_db.nVars());
        if (SolverOptions::opt_sort_variables) {
            std::vector<double> occ = getLiteralRelativeOccurrences();
            for (size_t i = 0; i < clause_db.nVars(); ++i) {
                activity[i] = occ[Lit(i, true)] + occ[Lit(i, false)];
                polarity[i] = occ[Lit(i, true)] < occ[Lit(i, false)];
            }
        }
        reset();

    }

    void setPolarity(Var v, bool sign) override {
        polarity[v] = sign;
    }

    Lit getLastDecision() override {
        return trail[*(trail.trail_lim.rbegin())];
    }

    std::vector<double> getLiteralRelativeOccurrences() const {
        std::vector<double> literalOccurrence(trail.nVars()*2, 0.0);

        if (literalOccurrence.size() > 0) {
            for (Clause* c : clause_db) {
                for (Lit lit : *c) {
                    literalOccurrence[lit] += 1.0 / c->size();
                }
            }
            double max = *std::max_element(literalOccurrence.begin(), literalOccurrence.end());
            for (double& occ : literalOccurrence) {
                occ = occ / max;
            }
        }
        
        return literalOccurrence;
    }

    void reset() override {
        std::vector<int> vs;
        for (Var v = 0; v < (Var)trail.nVars(); v++) {
            if (trail.isDecisionVar(v)) {
                vs.push_back(v);
            }
        }
        order_heap.build(vs);
    }

    void setActivity(Var v, double act) {
        activity[v] = act;
    }

    inline void varDecayActivity() {
        var_inc *= (1 / var_decay);
    }

    inline void varBumpActivity(Var v) {
        varBumpActivity(v, var_inc);
    }

    inline void varBumpActivity(Var v, double inc) {
        if ((activity[v] += inc) > 1e100) {
            varRescaleActivity();
        }
        if (order_heap.inHeap(v)) {
            order_heap.decrease(v); // update order-heap
        }
    }

    void varRescaleActivity() {
        for (size_t i = 0; i < activity.size(); ++i) {
            activity[i] *= 1e-100;
        }
        var_inc *= 1e-100;
    }

    void process_conflict() override {
        if (clause_db.result.nConflicts % 5000 == 0 && var_decay < max_var_decay) {
            var_decay += 0.01;
        }

        stamp.clear();
        for (Reason reason : clause_db.result.involved_clauses) {
            for (Lit lit : reason) {
                Var v = lit.var();
                if (!stamp[v]) {
                    stamp.set(v);
                    varBumpActivity(v);
                }
            }
        }

        varDecayActivity();

        add_back(trail.conflict_rbegin(), trail.rbegin());
    }

    template<typename Iterator>
    void add_back(Iterator rbegin, Iterator rend) {
        for (auto it = rbegin; it < rend; it++) {
            Lit lit = *it;
            Var v = lit.var();
            polarity[v] = lit.sign();
            if (!order_heap.inHeap(v) && trail.isDecisionVar(v))
                order_heap.insert(v);
        }
    }

    inline Lit pickBranchLit() override {
        Var next = var_Undef;

        // Activity based decision:
        while (next == var_Undef || trail.value(next) != l_Undef || !trail.isDecisionVar(next)) {
            if (order_heap.empty()) {
                next = var_Undef;
                break;
            }
            else {
                next = order_heap.removeMin();
            }
        }
        return next == var_Undef ? lit_Undef : Lit(next, polarity[next]);
    }
};

}
#endif /* SRC_CANDY_CORE_VSIDS_H_ */
