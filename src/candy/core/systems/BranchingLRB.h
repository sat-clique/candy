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

#ifndef SRC_CANDY_CORE_LRB_H_
#define SRC_CANDY_CORE_LRB_H_

#include <vector>

#include "candy/mtl/Heap.h"
#include "candy/core/Trail.h"
#include "candy/core/CNFProblem.h"
#include "candy/core/clauses/ClauseDatabase.h"
#include "candy/core/systems/BranchingInterface.h"

namespace Candy {

class BranchingLRB : public BranchingInterface {
private:
    ClauseDatabase& clause_db;
    Trail& trail;

public:
    struct VarOrderLt {
        std::vector<double>& weight;
        bool operator()(Var x, Var y) const {
            return weight[x] > weight[y];
        }
        VarOrderLt(std::vector<double>& act) : weight(act) {}
    };

    BranchingLRB(ClauseDatabase& _clause_db, Trail& _trail) :
        clause_db(_clause_db), trail(_trail), 
        order_heap(VarOrderLt(weight)), 
        weight(), polarity(), stamp(), 
        interval_assigned(), participated(), 
        step_size(SolverOptions::opt_lrb_step_size), 
        min_step_size(SolverOptions::opt_lrb_min_step_size) 
    {
        weight.resize(clause_db.nVars(), initial_weight);
        polarity.resize(clause_db.nVars(), initial_polarity);
        interval_assigned.resize(clause_db.nVars(), 0);
        participated.resize(clause_db.nVars(), 0);
        stamp.grow(clause_db.nVars());
        order_heap.grow(clause_db.nVars());
        if (SolverOptions::opt_sort_variables) {
            std::vector<double> occ = getLiteralRelativeOccurrences();
            for (size_t i = 0; i < clause_db.nVars(); ++i) {
                weight[i] = occ[Lit(i, true)] + occ[Lit(i, false)];
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


    void process_conflict() override {
        stamp.clear();
        for (Reason reason : clause_db.result.involved_clauses) { 
            for (Lit lit : reason) {
                Var v = lit.var();
                if (!stamp[v]) {
                    stamp.set(v);
                    participated[v]++;
                }
            }
        }
        if (step_size > min_step_size) {
            step_size -= 10e-6;
        }
        for (auto it = trail.begin(0); it != trail.end(); it++) {
            interval_assigned[it->var()]++;
        }
        //Todo: penalize all var not on trail

        double inv_step_size = 1.0 - step_size;
        for (auto it = trail.conflict_rbegin(); it < trail.rbegin(); it++) {
            Var v = it->var();
            if (interval_assigned[v] > 0) {
                weight[v] = inv_step_size * weight[v] + step_size * (participated[v] / interval_assigned[v]);
                interval_assigned[v] = 0;
                participated[v] = 0;
            }
        }

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

        // Weight based decision:
        while (next == var_Undef || trail.value(next) != l_Undef || !trail.isDecisionVar(next)) {
            if (order_heap.empty()) {
                next = var_Undef;
                break;
            } else {
                next = order_heap.removeMin();
            }
        }

        return next == var_Undef ? lit_Undef : Lit(next, polarity[next]);
    }

private:
    Glucose::Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable weigh.
    std::vector<double> weight; // A heuristic measurement of the weigh of a variable.
    std::vector<char> polarity; // The preferred polarity of each variable.
    Stamp<uint32_t> stamp;

    std::vector<uint32_t> interval_assigned;
    std::vector<uint32_t> participated;

    double step_size; 
    double min_step_size; 
    double initial_weight = 0.0;
    bool initial_polarity = true;

};

}
#endif /* SRC_CANDY_CORE_LRB_H_ */
