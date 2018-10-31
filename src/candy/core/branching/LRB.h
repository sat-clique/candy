/*
 * Branch.h
 *
 *  Created on: 25.08.2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_LRB_H_
#define SRC_CANDY_CORE_LRB_H_

#include <vector>

#include "candy/mtl/Heap.h"
#include "candy/core/Trail.h"
#include "candy/core/CNFProblem.h"
#include "candy/utils/CheckedCast.h"
#include "candy/core/ConflictAnalysis.h"
#include "candy/core/branching/BranchingInterface.h"

namespace Candy {

class LRB : public BranchingInterface<LRB> {
private:
    Trail& trail;
    ConflictAnalysis& analysis;

public:
    struct VarOrderLt {
        std::vector<double>& weight;
        bool operator()(Var x, Var y) const {
            return weight[x] > weight[y];
        }
        VarOrderLt(std::vector<double>& act) : weight(act) {}
    };

    LRB(Trail& _trail, ConflictAnalysis& _analysis, double _step_size = 0.4) :
        trail(_trail), analysis(_analysis), 
        order_heap(VarOrderLt(weight)),
        weight(), polarity(), decision(), stamp(), 
        interval_assigned(), participated(), 
        step_size(_step_size) {

    }

    LRB(LRB&& other) :
        trail(other.trail), analysis(other.analysis), 
        order_heap(VarOrderLt(weight)) {
            weight = std::move(other.weight);
            polarity = std::move(other.polarity);
            decision = std::move(other.decision);
            interval_assigned = std::move(other.interval_assigned);
            participated = std::move(other.participated);
            step_size = other.step_size;
            stamp.grow(other.stamp.size());
	}

    LRB& operator=(LRB&& other) {
        weight = std::move(other.weight);
        polarity = std::move(other.polarity);
        decision = std::move(other.decision);
        interval_assigned = std::move(other.interval_assigned);
        participated = std::move(other.participated);
        step_size = other.step_size;
        stamp.grow(other.stamp.size());
		return *this;
    }

    void setPolarity(Var v, bool sign) {
        polarity[v] = sign;
    }

    Lit getLastDecision() {
        return trail[*(trail.trail_lim.rbegin())];
    }

    // Declare if a variable should be eligible for selection in the decision heuristic.
    void setDecisionVar(Var v, bool b) {
        if (decision[v] != static_cast<char>(b)) {
            decision[v] = b;
            if (b) {
                insertVarOrder(v);
            }
        }
    }

    bool isDecisionVar(Var v) {
        return decision[v]; 
    }

    void grow() {
        decision.push_back(true);
        polarity.push_back(initial_polarity);
        weight.push_back(initial_weight);
        interval_assigned.push_back(0);
        participated.push_back(0);
        stamp.grow();
        insertVarOrder(decision.size() - 1);
    }

    void grow(size_t size) {
        int prevSize = decision.size(); // can be negative during initialization
        if (size > decision.size()) {
            decision.resize(size, true);
            polarity.resize(size, initial_polarity);
            weight.resize(size, initial_weight);
            interval_assigned.resize(size, 0);
            participated.resize(size, 0);
            stamp.grow(size);
            for (int i = prevSize; i < static_cast<int>(size); i++) {
                insertVarOrder(i);
            }
        }
    }

    void initFrom(const CNFProblem& problem) {
        std::vector<double> occ = problem.getLiteralRelativeOccurrences();
        for (size_t i = 0; i < decision.size(); i++) {
            weight[i] = occ[mkLit(i, true)] + occ[mkLit(i, false)];
            polarity[i] = occ[mkLit(i, true)] < occ[mkLit(i, false)];
        }
        rebuildOrderHeap();
    }


    void notify_conflict() {
        stamp.clear();
        for (Clause* clause : analysis.getResult().involved_clauses) {
            for (Lit lit : *clause) {
                Var v = var(lit);
                if (!stamp[v] && trail.level(v) > 0) {
                    stamp.set(v);
                    participated[v]++;
                }
            }
        }
        if (step_size > 0.06) {
            step_size -= 10e-6;
        }
        for (Lit lit : trail) {
            interval_assigned[var(lit)]++;
        }
        //Todo: penalize all var not on trail
    }

    void notify_backtracked() {
        double inv_step_size = 1.0 - step_size;
        for (Lit lit : trail.getBacktracked()) {
            Var v = var(lit);
            polarity[v] = sign(lit);

            if (interval_assigned[v] > 0) {
                weight[v] = inv_step_size * weight[v] + step_size * (participated[v] / interval_assigned[v]);
                interval_assigned[v] = 0;
                participated[v] = 0;
            }

            insertVarOrder(v);
        }
    }

    void notify_restarted() {
        std::fill(participated.begin(), participated.end(), 0);
        std::fill(interval_assigned.begin(), interval_assigned.end(), 0);
        rebuildOrderHeap();
    }

    inline Lit pickBranchLit() {
        Var next = var_Undef;

        // Activity based decision:
        while (next == var_Undef || trail.value(next) != l_Undef || !decision[next]) {
            if (order_heap.empty()) {
                next = var_Undef;
                break;
            } else {
                next = order_heap.removeMin();
            }
        }

        return next == var_Undef ? lit_Undef : mkLit(next, polarity[next]);
    }

private:
    Glucose::Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable weigh.
    std::vector<double> weight; // A heuristic measurement of the weigh of a variable.
    std::vector<char> polarity; // The preferred polarity of each variable.
    std::vector<char> decision; // Declares if a variable is eligible for selection in the decision heuristic
    Stamp<uint32_t> stamp;

    std::vector<uint32_t> interval_assigned;
    std::vector<uint32_t> participated;

    double step_size; // Amount to bump next variable with.
    double initial_weight = 0.0;
    bool initial_polarity = true;

    // Insert a variable in the decision order priority queue.
    inline void insertVarOrder(Var x) {
        if (!order_heap.inHeap(x) && decision[x])
            order_heap.insert(x);
    }

    void rebuildOrderHeap() {
        vector<Var> vs;
        for (size_t v = 0; v < decision.size(); v++) {
            if (decision[v] && !trail.isAssigned(v)) {
                vs.push_back(checked_unsignedtosigned_cast<size_t, Var>(v));
            }
        }
        order_heap.build(vs);
    }
};

}
#endif /* SRC_CANDY_CORE_LRB_H_ */
