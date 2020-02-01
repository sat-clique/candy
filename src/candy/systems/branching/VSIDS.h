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
#include "candy/utils/CheckedCast.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/systems/branching/BranchingDiversificationInterface.h"

namespace Candy
{

class VSIDS : public BranchingDiversificationInterface
{
protected:
    ClauseDatabase &clause_db;
    Trail &trail;

public:
    struct VarOrderLt
    {
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
    const bool glucose_style_extra_bump = false;
    const bool initial_polarity = true;
    const double initial_activity = 0.0;

    VSIDS(ClauseDatabase &_clause_db, Trail &_trail,
          double _var_decay = SolverOptions::opt_vsids_var_decay,
          double _max_var_decay = SolverOptions::opt_vsids_max_var_decay,
          bool _glucose_style_extra_bump = SolverOptions::opt_vsids_extra_bump) : clause_db(_clause_db), trail(_trail),
                                                                                  order_heap(VarOrderLt(activity)),
                                                                                  activity(), polarity(), stamp(),
                                                                                  var_inc(1), var_decay(_var_decay), max_var_decay(_max_var_decay),
                                                                                  glucose_style_extra_bump(_glucose_style_extra_bump)
    {
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
        if (SolverOptions::opt_sort_variables)
        {
            std::vector<double> occ = getLiteralRelativeOccurrences();
            for (size_t i = 0; i < trail.nVars(); ++i)
            {
                activity[i] = occ[Lit(i, true)] + occ[Lit(i, false)];
                polarity[i] = occ[Lit(i, true)] < occ[Lit(i, false)];
            }
        }
        reset();
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

    /**
     * Branching Diversification Interface for HordeSAT integration
     * */
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

    /* Calculate Relative Occurences of Literals
     * per clause: literal_occurence = 1 / clause_size
     * overall relative occurence is sum of literal_occurence in clauses
     */
    std::vector<double> getLiteralRelativeOccurrences() const
    {
        std::vector<double> literalOccurrence(trail.nVars() * 2, 0.0);

        if (literalOccurrence.size() > 0)
        {
            for (Clause *c : clause_db)
            {
                for (Lit lit : *c)
                {
                    literalOccurrence[lit] += 1.0 / c->size();
                }
            }
            double max = *std::max_element(literalOccurrence.begin(), literalOccurrence.end());
            for (double &occ : literalOccurrence)
            {
                occ = occ / max;
            }
        }

        return literalOccurrence;
    }

    // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    inline void varDecayActivity()
    {
        var_inc *= (1 / var_decay);
    }

    // Increase a variable with the current 'bump' value.
    inline void varBumpActivity(Var v)
    {
        varBumpActivity(v, var_inc);
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
        return next == var_Undef ? lit_Undef : Lit(next, polarity[next]);
    }
};

} // namespace Candy
#endif /* SRC_CANDY_CORE_VSIDS_H_ */
