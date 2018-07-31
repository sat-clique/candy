/*
 * Branch.h
 *
 *  Created on: 25.08.2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_VSIDS_H_
#define SRC_CANDY_CORE_VSIDS_H_

#include <vector>

#include "candy/mtl/Heap.h"
#include "candy/core/Trail.h"
#include "candy/core/CNFProblem.h"
#include "candy/utils/CheckedCast.h"
#include "candy/core/ConflictAnalysis.h"
#include "candy/core/branching/BranchingInterface.h"

namespace Candy {

class VSIDS : public BranchingInterface<VSIDS> {
public:
    struct VarOrderLt {
        std::vector<double>& activity;
        bool operator()(Var x, Var y) const {
            return activity[x] > activity[y];
        }
        VarOrderLt(std::vector<double>& act) : activity(act) {}
    };

    Glucose::Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable activity.
    std::vector<double> activity; // A heuristic measurement of the activity of a variable.
    std::vector<char> polarity; // The preferred polarity of each variable.
    std::vector<char> decision; // Declares if a variable is eligible for selection in the decision heuristic
    Stamp<uint32_t> stamp;
    double var_inc; // Amount to bump next variable with.
    double var_decay;
    double max_var_decay;
    bool initial_polarity = true;
    double initial_activity = 0.0;

    VSIDS(double _var_decay = 0.8, double _max_var_decay = 0.95) :
        order_heap(VarOrderLt(activity)),
        activity(), polarity(), decision(), stamp(), 
        var_inc(1), var_decay(_var_decay), max_var_decay(_max_var_decay) {

    }

    VSIDS(VSIDS&& other) : order_heap(VarOrderLt(activity)) {
        activity = std::move(other.activity);
        polarity = std::move(other.polarity);
        decision = std::move(other.decision);
		var_inc = other.var_inc;
		var_decay = other.var_decay;
        max_var_decay = other.max_var_decay;
        stamp.incSize(other.stamp.size());
	}

    VSIDS& operator=(VSIDS&& other) {
//        order_heap(VarOrderLt(activity));
        activity = std::move(other.activity);
        polarity = std::move(other.polarity);
        decision = std::move(other.decision);
        var_inc = other.var_inc;
        var_decay = other.var_decay;
        max_var_decay = other.max_var_decay;
        stamp.incSize(other.stamp.size());
		return *this;
    }

    void setPolarity(Var v, bool pol) {
        polarity[v] = pol;
    }

    void setActivity(Var v, double act) {
        activity[v] = act;
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
        activity.push_back(initial_activity);
        stamp.incSize();
        insertVarOrder(decision.size() - 1);
    }

    void grow(size_t size) {
        int prevSize = decision.size(); // can be negative during initialization
        if (size > decision.size()) {
            decision.resize(size, true);
            polarity.resize(size, initial_polarity);
            activity.resize(size, initial_activity);
            stamp.incSize(size);
            for (int i = prevSize; i < static_cast<int>(size); i++) {
                insertVarOrder(i);
            }
        }
    }

    void initFrom(const CNFProblem& problem) {
        std::vector<double> occ = problem.getLiteralRelativeOccurrences();
        for (size_t i = 0; i < decision.size(); i++) {
            activity[i] = occ[mkLit(i, true)] + occ[mkLit(i, false)];
            polarity[i] = occ[mkLit(i, true)] < occ[mkLit(i, false)];
        }
        rebuildOrderHeap();
    }

    // Insert a variable in the decision order priority queue.
    inline void insertVarOrder(Var x) {
        if (!order_heap.inHeap(x) && decision[x])
            order_heap.insert(x);
    }

    // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    inline void varDecayActivity() {
        var_inc *= (1 / var_decay);
    }

    // Increase a variable with the current 'bump' value.
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
        for (size_t i = 0; i < activity.size(); i++) {
            activity[i] *= 1e-100;
        }
        var_inc *= 1e-100;
    }

    void rebuildOrderHeap() {
        vector<Var> vs;
        for (size_t v = 0; v < decision.size(); v++) {
            if (decision[v]) {
                vs.push_back(checked_unsignedtosigned_cast<size_t, Var>(v));
            }
        }
        order_heap.build(vs);
    }

    void rebuildOrderHeap(Trail& trail) {
        vector<Var> vs;
        for (size_t v = 0; v < decision.size(); v++) {
            if (decision[v] && trail.value(checked_unsignedtosigned_cast<size_t, Var>(v)) == l_Undef) {
                vs.push_back(checked_unsignedtosigned_cast<size_t, Var>(v));
            }
        }
        order_heap.build(vs);
    }


    void notify_conflict(AnalysisResult ana, Trail& trail, unsigned int learnt_lbd) {
        if (ana.nConflicts % 5000 == 0 && var_decay < max_var_decay) {
            var_decay += 0.01;
        }

        stamp.clear();
        for (Clause* clause : ana.involved_clauses) {
            for (Lit lit : *clause) {
                Var v = var(lit);
                if (!stamp[v] && trail.level(v) > 0) {
                    stamp.set(v);
                    varBumpActivity(v);
                    if (trail.level(v) >= (int)trail.decisionLevel() && trail.reason(v) != nullptr && trail.reason(v)->isLearnt()) {
                        // UPDATEVARACTIVITY trick (see competition'09 companion paper)
                        if (trail.reason(v)->getLBD() < learnt_lbd) {
                            varBumpActivity(v);
                        }
                    }
                }
            }
        }

        varDecayActivity();
    }

    void notify_backtracked(std::vector<Lit> lits) {
        for (Lit lit : lits) {
            polarity[var(lit)] = sign(lit);
            insertVarOrder(var(lit));
        }
    }

    void notify_restarted(Trail& trail) {
        rebuildOrderHeap(trail);
    }

    inline Lit pickBranchLit(Trail& trail) {
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
};

}
#endif /* SRC_CANDY_CORE_VSIDS_H_ */
