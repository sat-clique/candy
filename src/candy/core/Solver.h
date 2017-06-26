/***************************************************************************************[Solver.h]
 Glucose -- Copyright (c) 2009-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 LRI  - Univ. Paris Sud, France (2009-2013)
 Labri - Univ. Bordeaux, France

 Syrup (Glucose Parallel) -- Copyright (c) 2013-2014, Gilles Audemard, Laurent Simon
 CRIL - Univ. Artois, France
 Labri - Univ. Bordeaux, France

 Glucose sources are based on MiniSat (see below MiniSat copyrights). Permissions and copyrights of
 Glucose (sources until 2013, Glucose 3.0, single core) are exactly the same as Minisat on which it
 is based on. (see below).

 Glucose-Syrup sources are based on another copyright. Permissions and copyrights for the parallel
 version of Glucose-Syrup (the "Software") are granted, free of charge, to deal with the Software
 without restriction, including the rights to use, copy, modify, merge, publish, distribute,
 sublicence, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 - The above and below copyrights notices and this permission notice shall be included in all
 copies or substantial portions of the Software;
 - The parallel version of Glucose (all files modified since Glucose 3.0 releases, 2013) cannot
 be used in any competitive event (sat competitions/evaluations) without the express permission of
 the authors (Gilles Audemard / Laurent Simon). This is also the case for any competitive event
 using Glucose Parallel as an embedded SAT engine (single core or not).


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
 **************************************************************************************************/

#ifndef Glucose_Solver_h
#define Glucose_Solver_h

#include <vector>
#include <unordered_map>
#include <math.h>
#include <string>
#include <type_traits>
#include <memory>
#include <limits>

#include <candy/core/Statistics.h>
#include "candy/mtl/Heap.h"
#include "candy/utils/Options.h"
#include "candy/core/SolverTypes.h"
#include "candy/mtl/BoundedQueue.h"
#include "candy/core/Clause.h"
#include "candy/core/Certificate.h"
#include "candy/core/ClauseAllocator.h"
#include <candy/core/CNFProblem.h>
#include "candy/utils/System.h"
#include "candy/utils/Attributes.h"
#include "candy/utils/CheckedCast.h"

#include "sonification/SolverSonification.h"

#define FUTURE_PROPAGATE

namespace Candy {

class DefaultPickBranchLit {
public:
    class Parameters {
        
    };
    
    DefaultPickBranchLit() noexcept {
        
    }
    
    explicit DefaultPickBranchLit(const Parameters& params) noexcept {
        (void)params;
    }
};

/**
 * \tparam PickBranchLitT   the PickBranchLit type used to choose a
 *   strategy for determining decision (ie. branching) literals.
 *   PickBranchLitT must satisfy the following conditions:
 *    - PickBranchLitT must be a class type.
 *    - PickBranchLitT::Parameters must be a class type.
 *    - PickBranchLitT must have a zero-argument constructor.
 *    - PickBranchLitT must have a constructor taking a single argument of type
 *        const Parameters& params.
 *    - PickBranchLitT must be move-assignable.
 *    - PickBranchLitT must be move-constructible.
 *    - There must be a specialization of Solver::pickBranchLit<PickBranchLitT>.
 */
template<class PickBranchLitT = DefaultPickBranchLit>
class Solver {
    static_assert(std::is_class<PickBranchLitT>::value, "PickBranchLitT must be a class");
    static_assert(std::is_class<typename PickBranchLitT::Parameters>::value,
                  "PickBranchLitT::Parameters must be a class");
    static_assert(std::is_constructible<PickBranchLitT>::value,
                  "PickBranchLitT must have a constructor without arguments");
    static_assert(std::is_move_assignable<PickBranchLitT>::value,
                  "PickBranchLitT must be move-assignable");
    static_assert(std::is_move_constructible<PickBranchLitT>::value,
                  "PickBranchLitT must be move-constructible");

    friend class SolverConfiguration;

public:
    using PickBranchLitType = PickBranchLitT;
    
    Solver();
    virtual ~Solver();
    
    /// Reinitializes the decision literal picking strategy using the given parameters.
    void initializePickBranchLit(const typename PickBranchLitT::Parameters& params);
    
    // Add a new variable with parameters specifying variable mode.
    virtual Var newVar(bool polarity = true, bool dvar = true, double activity = 0.0);

    // Add clauses to the solver
    virtual void addClauses(const CNFProblem& dimacs);

    template<typename Iterator>
    bool addClause(Iterator begin, Iterator end);

    inline bool addClause(const vector<Lit>& ps) {
        return addClause(ps.begin(), ps.end());
    }

    inline bool addClause(std::initializer_list<Lit> lits) {
        return addClause(lits.begin(), lits.end());
    }

    inline void printDIMACS() {
        printf("p cnf %zu %zu\n", nVars(), clauses.size());
        for (auto clause : clauses) {
            clause->print();
        }
    }

    // use with care (written for solver tests only)
    Clause& getClause(size_t pos) {
        assert(pos < clauses.size());
        return *clauses[pos];
    }

    vector<Lit>& getConflict() {
        return conflict;
    }

    // Solving:
    bool simplify(); // remove satisfied clauses
    bool strengthen(); // remove false literals from clauses
    virtual bool eliminate(bool use_asymm, bool use_elim);  // Perform variable elimination based simplification.
    virtual lbool solve(); // Main solve method (assumptions given in 'assumptions').

    inline lbool solve(std::initializer_list<Lit> assumps) {
        assumptions.clear();
        assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
        return solve();
    }

    inline lbool solve(const vector<Lit>& assumps) {
        assumptions.clear();
        assumptions.insert(assumptions.end(), assumps.begin(), assumps.end());
        return solve();
    }

    // true means solver is in a conflicting state
    inline bool isInConflictingState() const {
        return !ok;
    }

    // Declare if a variable should be eligible for selection in the decision heuristic.
    inline void setDecisionVar(Var v, bool b) {
        if (decision[v] != static_cast<char>(b)) {
            decision[v] = b;
            if (b) {
                insertVarOrder(v);
                Statistics::getInstance().solverDecisionVariablesInc();
            } else {
                Statistics::getInstance().solverDecisionVariablesDec();
            }
        }
    }

    // The value of a variable in the last model. The last call to solve must have been satisfiable.
    lbool modelValue(Var x) const {
        return model[x];
    }
    // The value of a literal in the last model. The last call to solve must have been satisfiable.
    lbool modelValue(Lit p) const {
        return model[var(p)] ^ sign(p);
    }
    inline size_t nClauses() const {
        return clauses.size();
    }
    inline size_t nLearnts() const {
        return learnts.size() + persist.size();
    }

    inline size_t nVars() const {
        return trail_abstraction.vardata.size();
    }

    inline bool isSelector(Var v) {
        return (incremental && (uint32_t)v >= nbVarsInitialFormula);
    }

    // Incremental mode
    void setIncrementalMode();
    void initNbInitialVars(int nb);
    bool isIncremental();

    // Resource constraints:
    void setConfBudget(uint64_t x) {
        conflict_budget = nConflicts + x;
    }
    void setPropBudget(uint64_t x) {
        propagation_budget = nPropagations + x;
    }
    void setInterrupt(bool value) {
        asynch_interrupt = value;
    }
    void budgetOff() {
        conflict_budget = propagation_budget = 0;
    }

    //TODO: use std::function<int(void*)> as type here
    void setTermCallback(void* state, int (*termCallback)(void*)) {
        this->termCallbackState = state;
        this->termCallback = termCallback;
    }

    void setLearntCallback(void* state, int max_length, void (*learntCallback)(void* state, int* clause)) {
        this->learntCallbackState = state;
        this->learntCallbackMaxLength = max_length;
        this->learntCallback = learntCallback;
    }

    void setCertificate(Certificate& certificate) {
        this->certificate = &certificate;
    }

    void setVerbosities(int verbEveryConflicts, int verbosity) {
        assert(verbosity == 0 || verbEveryConflicts > 0);
        this->verbEveryConflicts = verbEveryConflicts;
        this->verbosity = verbosity;
    }

    // Certified UNSAT (Thanks to Marijn Heule)
    Certificate defaultCertificate;
    Certificate* certificate;

    // a few stats are used for heuristics control, keep them here
    uint64_t nConflicts, nPropagations;

    // Control verbosity
    int verbEveryConflicts;
    int verbosity;

    // Extra results: (read-only member variable)
    vector<lbool> model; // If problem is satisfiable, this vector contains the model (if any).
    vector<Lit> conflict; // If problem is unsatisfiable (possibly under assumptions), this vector represent the final conflict clause expressed in the assumptions.

protected:
    // Helper structures:
    struct VarData {
        Clause* reason;
        uint_fast32_t level;
        VarData() :
            reason(nullptr), level(0) {}
        VarData(Clause* _reason, uint_fast32_t _level) :
            reason(_reason), level(_level) {}
    };

    struct Watcher {
        Clause* cref;
        Lit blocker;
        Watcher() :
            cref(nullptr), blocker(lit_Undef) {}
        Watcher(Clause* cr, Lit p) :
            cref(cr), blocker(p) {}
        bool operator==(const Watcher& w) const {
            return cref == w.cref;
        }
        bool operator!=(const Watcher& w) const {
            return cref != w.cref;
        }
    };

    struct WatcherDeleted {
        WatcherDeleted() { }
        inline bool operator()(const Watcher& w) const {
            return w.cref->isDeleted() == 1;
        }
    };

    struct VarOrderLt {
        const vector<double>& activity;
        bool operator()(Var x, Var y) const {
            return activity[x] > activity[y];
        }
        VarOrderLt(const vector<double>& act) : activity(act) {}
    };

    struct reduceDB_lt {
        reduceDB_lt() {
        }

        bool operator()(Clause* x, Clause* y) {
    //        // XMiniSat paper, alternate ordering
    //        const uint8_t reduceOnSizeSize = 12;
    //        uint32_t w1 = x->size() < reduceOnSizeSize : x->size() : x->size() + x->getLBD();
    //        uint32_t w2 = y->size() < reduceOnSizeSize : y->size() : y->size() + y->getLBD();
    //        return w1 > w2 || (w1 == w2 && x->activity() < y->activity());
            return x->getLBD() > y->getLBD() || (x->getLBD() == y->getLBD() && x->activity() < y->activity());
        }
    };

    ClauseAllocator allocator;

#ifndef FUTURE_PROPAGATE
#define NWATCHES 2
#else
#define NWATCHES 6
#endif

    std::array<OccLists<Lit, Watcher, WatcherDeleted>, NWATCHES> watches;

    class TrailAbstraction {
    public:
        TrailAbstraction() : assigns(), trail(), trail_size(0), qhead(0), vardata(), trail_lim() {

        }
        vector<lbool> assigns; // The current assignments.
        vector<Lit> trail; // Assignment stack; stores all assigments made in the order they were made.
        uint32_t trail_size; // Current number of assignments (used to optimize propagate, through getting rid of capacity checking)
        uint32_t qhead; // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
        vector<VarData> vardata; // Stores reason and level for each variable.
        vector<uint32_t> trail_lim; // Separator indices for different decision levels in 'trail'.

        // The current value of a variable.
        inline lbool value(Var x) const {
            return assigns[x];
        }

        // The current value of a literal.
        inline lbool value(Lit p) const {
            return assigns[var(p)] ^ sign(p);
        }

        inline size_t nAssigns() const {
            return trail_size;
        }

        // Main internal methods:
        inline Clause* reason(Var x) const {
            return vardata[x].reason;
        }

        inline int level(Var x) const {
            return vardata[x].level;
        }

        // Begins a new decision level
        inline void newDecisionLevel() {
            trail_lim.push_back(trail_size);
        }

        // Returns TRUE if a clause is a reason for some implication in the current state.
        inline bool locked(Clause* cr) const {
            Clause& c = *cr;
            if (c.size() > 2) return value(c[0]) == l_True && reason(var(c[0])) == cr;
            return (value(c[0]) == l_True && reason(var(c[0])) == cr) || (value(c[1]) == l_True && reason(var(c[1])) == cr);
        }

        // Returns TRUE if a clause is satisfied in the current state.
        inline bool satisfied(const Clause& c) const {
            return std::any_of(c.begin(), c.end(), [this] (Lit lit) { return value(lit) == l_True; });
        }

        // Gives the current decisionlevel.
        inline uint32_t decisionLevel() const {
            return checked_unsigned_cast<size_t, uint32_t>(trail_lim.size());
        }

        inline void uncheckedEnqueue(Lit p, Clause* from = nullptr) {
            assert(value(p) == l_Undef);
            assigns[var(p)] = lbool(!sign(p));
            vardata[var(p)] = VarData(from, decisionLevel());
            trail[trail_size++] = p;
        }
    } trail_abstraction;

    vector<char> polarity; // The preferred polarity of each variable.
    vector<char> decision; // Declares if a variable is eligible for selection in the decision heuristic.
    vector<Lit> assumptions; // Current set of assumptions provided to solve by the user.

    // for activity based heuristics
    Glucose::Heap<VarOrderLt> order_heap; // A priority queue of variables ordered with respect to the variable activity.
    vector<double> activity; // A heuristic measurement of the activity of a variable.
    double var_inc; // Amount to bump next variable with.
    double var_decay;
    double max_var_decay;
    double cla_inc; // Amount to bump next clause with.
    double clause_decay;

    // Clauses
    vector<Clause*> clauses; // List of problem clauses.
    vector<Clause*> learnts; // List of learnt clauses.
    vector<Clause*> persist; // List of binary learnt clauses.

    // Constants For restarts
    double K;
    double R;
    float sumLBD = 0; // used to compute the global average of LBD. Restarts...
    // Bounded queues for restarts
    bqueue<uint32_t> lbdQueue, trailQueue;

    // used for reduceDB
    uint64_t curRestart;
    uint32_t nbclausesbeforereduce; // To know when it is time to reduce clause database
    uint16_t incReduceDB;
    uint16_t persistentLBD;
    uint16_t lbLBDFrozenClause;

    // Constant for reducing clause
    uint16_t lbSizeMinimizingClause;
    uint16_t lbLBDMinimizingClause;

    // constants for memory reorganization
    uint8_t revamp;
    bool sort_watches;
    bool sort_variables;

    bool new_unary; // Indicates whether a unary clause was learnt since the last restart
    bool ok; // If FALSE, the constraints are already unsatisfiable. No part of the solver state may be used!

    // Variables added for incremental mode
    bool incremental; // Use incremental SAT Solver
    uint32_t nbVarsInitialFormula; // nb VAR in formula without assumptions (incremental SAT)

    // inprocessing
    uint64_t lastRestartWithInprocessing;
    uint32_t inprocessingFrequency;

    // lbd
    vector<uint32_t> permDiff; // permDiff[var] contains the current conflict number... Used to count the number of  LBD
    uint32_t MYFLAG;

    // temporaries (to reduce allocation overhead)
    vector<char> seen;
    vector<Var> analyze_clear;
    vector<Var> analyze_stack;

    // Resource contraints and other interrupts
    uint32_t conflict_budget;    // 0 means no budget.
    uint32_t propagation_budget; // 0 means no budget.
    void* termCallbackState;
    int (*termCallback)(void* state);
    bool asynch_interrupt;

    // Learnt callback ipasir
    void* learntCallbackState;
    int learntCallbackMaxLength;
    void (*learntCallback)(void* state, int* clause);

    // Sonification
    SolverSonification sonification;
    
    // Variables for decisions
    PickBranchLitT pickBranchLitData;

    inline uint64_t abstractLevel(Var x) const {
        return 1ull << (trail_abstraction.level(x) % 64);
    }

    // Insert a variable in the decision order priority queue.
    inline void insertVarOrder(Var x) {
        if (!order_heap.inHeap(x) && decision[x])
            order_heap.insert(x);
    }

    // Operations on clauses:
    void attachClause(Clause* cr); // Attach a clause to watcher lists.
    void detachClause(Clause* cr, bool strict = false); // Detach a clause to watcher lists.
    void removeClause(Clause* cr, bool strict_detach = false); // Detach and free a clause.
    void freeMarkedClauses(vector<Clause*>& list);

    template <typename Iterator>
    uint_fast16_t computeLBD(Iterator it, Iterator end);
    bool minimisationWithBinaryResolution(vector<Lit> &out_learnt);

    Lit pickBranchLit(); // Return the next decision variable.
    Lit defaultPickBranchLit(); // Return the next decision variable (default implementation).
    Clause* propagate(); // Perform unit propagation. Returns possibly conflicting clause.
    inline Clause* future_propagate_clauses(Lit p, uint_fast8_t n);
    void cancelUntil(int level); // Backtrack until a certain level.
    void analyze(Clause* confl, vector<Lit>& out_learnt, uint_fast16_t &nblevels);
    void analyzeFinal(Lit p, vector<Lit>& out_conflict); // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
    bool litRedundant(Lit p, uint64_t abstract_levels); // (helper method for 'analyze()')
    lbool search(); // Search for a given number of conflicts.
    virtual void reduceDB(); // Reduce the set of learnt clauses.
    void rebuildOrderHeap();
    void revampClausePool(uint8_t upper);

    // Decay all variables with the specified factor. Implemented by increasing the 'bump' value instead.
    inline void varDecayActivity() {
        var_inc *= (1 / var_decay);
    }

    // Increase a variable with the current 'bump' value.
    inline void varBumpActivity(Var v) {
        if (!isSelector(v)) {
            varBumpActivity(v, var_inc);
        }
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
        for (size_t i = 0; i < nVars(); i++) {
            activity[i] *= 1e-100;
        }
        var_inc *= 1e-100;
    }

    inline void claDecayActivity() {
        cla_inc *= (1 / clause_decay);
    }

    inline void claBumpActivity(Clause& c) {
        if ((c.activity() += static_cast<float>(cla_inc)) > 1e20f) {
            claRescaleActivity();
        }
    }

    void claRescaleActivity() {
        for (auto container : { clauses, learnts, persist }) {
            for (Clause* clause : container) {
                clause->activity() *= 1e-20f;
            }
        }
        cla_inc *= 1e-20;
    }

    inline bool withinBudget() {
        return !asynch_interrupt && (termCallback == nullptr || 0 == termCallback(termCallbackState))
                && (conflict_budget == 0 || nConflicts < conflict_budget) && (propagation_budget == 0 || nPropagations < propagation_budget);
    }
};

/**
 * \brief A readily forward-declarable Solver<>
 */
class DefaultSolver : public Solver<> {

};




//******************************************************************************
// Solver<PickBranchLitT> implementation
//******************************************************************************

namespace SolverOptions {
    using namespace Glucose;
    
    extern const char* _cat;
    extern const char* _cr;
    extern const char* _cred;
    extern const char* _cm;
    
    extern DoubleOption opt_K;
    extern DoubleOption opt_R;
    extern IntOption opt_size_lbd_queue;
    extern IntOption opt_size_trail_queue;
    
    extern IntOption opt_first_reduce_db;
    extern IntOption opt_inc_reduce_db;
    extern IntOption opt_persistent_lbd;
    extern IntOption opt_lb_lbd_frozen_clause;
    
    extern IntOption opt_lb_size_minimzing_clause;
    extern IntOption opt_lb_lbd_minimzing_clause;
    
    extern DoubleOption opt_var_decay;
    extern DoubleOption opt_max_var_decay;
    extern DoubleOption opt_clause_decay;
    extern IntOption opt_phase_saving;
    
    extern IntOption opt_sonification_delay;
    
    extern IntOption opt_revamp;
    extern BoolOption opt_sort_watches;
    extern BoolOption opt_sort_variables;
    extern IntOption opt_inprocessing;
}

template<class PickBranchLitT>
Solver<PickBranchLitT>::Solver() :
    // default certificate, used when none other is set
    defaultCertificate(nullptr, false),
    // unsat certificate
    certificate(&defaultCertificate),
    // stats for heuristic control
    nConflicts(0), nPropagations(0),
    // verbosity flags
    verbEveryConflicts(10000), verbosity(0),
    // results
    model(), conflict(),
    // clause allocator
    allocator(),
    // watchers
    watches(),
    // current assignment
    trail_abstraction(),
    polarity(), decision(),
    // assumptions
    assumptions(),
    // for activity based heuristics
    order_heap(VarOrderLt(activity)),
    activity(),
    var_inc(1), var_decay(SolverOptions::opt_var_decay), max_var_decay(SolverOptions::opt_max_var_decay),
    cla_inc(1), clause_decay(SolverOptions::opt_clause_decay),
    // clauses
    clauses(), learnts(), persist(),
    // restarts
    K(SolverOptions::opt_K), R(SolverOptions::opt_R), sumLBD(0),
    lbdQueue(SolverOptions::opt_size_lbd_queue), trailQueue(SolverOptions::opt_size_trail_queue),
    // reduce db heuristic control
    curRestart(0), nbclausesbeforereduce(SolverOptions::opt_first_reduce_db),
    incReduceDB(SolverOptions::opt_inc_reduce_db),
    persistentLBD(SolverOptions::opt_persistent_lbd),
    lbLBDFrozenClause(SolverOptions::opt_lb_lbd_frozen_clause),
    // clause reduction
    lbSizeMinimizingClause(SolverOptions::opt_lb_size_minimzing_clause),
    lbLBDMinimizingClause(SolverOptions::opt_lb_lbd_minimzing_clause),
    // memory reorganization
    revamp(SolverOptions::opt_revamp),
    sort_watches(SolverOptions::opt_sort_watches),
    sort_variables(SolverOptions::opt_sort_variables),
    // simplify
    new_unary(false),
    // conflict state
    ok(true),
    // incremental mode
    incremental(false), nbVarsInitialFormula(INT32_MAX),
    // inprocessing
    lastRestartWithInprocessing(0),
    inprocessingFrequency(SolverOptions::opt_inprocessing),
    // lbd computation
    permDiff(), MYFLAG(0),
    // temporaries
    seen(), analyze_clear(), analyze_stack(),
    // resource constraints and other interrupt related
    conflict_budget(0), propagation_budget(0),
    termCallbackState(nullptr), termCallback(nullptr),
    asynch_interrupt(false),
    // learnt callback ipasir
    learntCallbackState(nullptr), learntCallbackMaxLength(0), learntCallback(nullptr),
    // sonification
    sonification(),
    pickBranchLitData()
{
    for (auto& watchers : watches) {
        watchers = OccLists<Lit, Watcher, WatcherDeleted>();
    }
}

template<class PickBranchLitT>
Solver<PickBranchLitT>::~Solver() {
    for (Clause* c : clauses) {
        allocator.deallocate(c);
    }
    for (Clause* c : learnts) {
        allocator.deallocate(c);
    }
    for (Clause* c : persist) {
        allocator.deallocate(c);
    }
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::initializePickBranchLit(const typename PickBranchLitT::Parameters& params) {
    pickBranchLitData = PickBranchLitT(params);
}

/****************************************************************
 Set the incremental mode
 ****************************************************************/

// This function set the incremental mode to true.
// You can add special code for this mode here.
template<class PickBranchLitT>
void Solver<PickBranchLitT>::setIncrementalMode() {
    incremental = true;
}

// Number of variables without selectors
template<class PickBranchLitT>
void Solver<PickBranchLitT>::initNbInitialVars(int nb) {
    nbVarsInitialFormula = nb;
}

template<class PickBranchLitT>
bool Solver<PickBranchLitT>::isIncremental() {
    return incremental;
}

/***
 * Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
 * used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
 ***/
template<class PickBranchLitT>
Var Solver<PickBranchLitT>::newVar(bool sign, bool dvar, double act) {
    int v = static_cast<int>(nVars());
    for (auto& watchers : watches) {
        watchers.init(mkLit(v, true));
    }
    trail_abstraction.assigns.push_back(l_Undef);
    trail_abstraction.vardata.emplace_back();
    trail_abstraction.trail.resize(v + 1);
    seen.push_back(0);
    permDiff.push_back(0);
    decision.push_back(0);
    activity.push_back(act);
    polarity.push_back(sign);
    setDecisionVar(v, dvar);
    return v;
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::addClauses(const CNFProblem& dimacs) {
    const std::vector<std::vector<Lit>*>& problem = dimacs.getProblem();
    size_t curVars = this->nVars();
    size_t maxVars = (size_t)dimacs.nVars();
    if (maxVars > curVars) {
        for (auto& watchers : watches) {
            watchers.init(mkLit(maxVars, true));
        }
        trail_abstraction.assigns.resize(maxVars, l_Undef);
        trail_abstraction.vardata.resize(maxVars);
        trail_abstraction.trail.resize(maxVars);
        seen.resize(maxVars, 0);
        permDiff.resize(maxVars, 0);
        decision.resize(maxVars, true);
        activity.resize(maxVars, 0.0);
        polarity.resize(maxVars, true);
        if (sort_variables) {
            std::vector<double> occ = dimacs.getLiteralRelativeOccurrences();
            for (size_t i = curVars; i < maxVars; i++) {
                activity[i] = occ[mkLit(i, true)] + occ[mkLit(i, false)];
                polarity[i] = occ[mkLit(i, true)] > occ[mkLit(i, false)];
            }
        }
        for (size_t i = curVars; i < maxVars; i++) {
            insertVarOrder(i);
            Statistics::getInstance().solverDecisionVariablesInc();
        }
    }
    allocator.announceClauses(problem);
    for (vector<Lit>* clause : problem) {
        addClause(*clause);
        if (!ok) return;
    }
    ok = simplify() && strengthen();
}

template<class PickBranchLitT>
template<typename Iterator>
bool Solver<PickBranchLitT>::addClause(Iterator begin, Iterator end) {
    assert(trail_abstraction.decisionLevel() == 0);

    uint32_t size = static_cast<uint32_t>(std::distance(begin, end));

    if (size == 0) {
        return ok = false;
    }
    else if (size == 1) {
        if (trail_abstraction.value(*begin) == l_Undef) {
            trail_abstraction.uncheckedEnqueue(*begin);
            return ok = (propagate() == nullptr);
        }
        else if (trail_abstraction.value(*begin) == l_True) {
            trail_abstraction.vardata[var(*begin)].reason = nullptr;
            return ok;
        }
        else {
            return ok = false;
        }
    }
    else {
        Clause* cr = new (allocator.allocate(size)) Clause(begin, end);
        clauses.push_back(cr);
        attachClause(cr);
    }
    
    return ok;
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::attachClause(Clause* cr) {
    assert(cr->size() > 1);
    uint_fast8_t pos = std::min(cr->size()-2, NWATCHES-1);
    watches[pos][~cr->first()].emplace_back(cr, cr->second());
    watches[pos][~cr->second()].emplace_back(cr, cr->first());
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::detachClause(Clause* cr, bool strict) {
    assert(cr->size() > 1);
    uint_fast8_t pos = std::min(cr->size()-2, NWATCHES-1);
    if (strict) {
        vector<Watcher>& list0 = watches[pos][~cr->first()];
        vector<Watcher>& list1 = watches[pos][~cr->second()];
        list0.erase(std::remove_if(list0.begin(), list0.end(), [cr](Watcher w){ return w.cref == cr; }), list0.end());
        list1.erase(std::remove_if(list1.begin(), list1.end(), [cr](Watcher w){ return w.cref == cr; }), list1.end());
    } else {
        watches[pos].smudge(~cr->first());
        watches[pos].smudge(~cr->second());
    }
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::removeClause(Clause* cr, bool strict_detach) {
    certificate->removed(cr->begin(), cr->end());
    detachClause(cr, strict_detach);
    // Don't leave pointers to free'd memory!
    if (trail_abstraction.locked(cr)) {
        trail_abstraction.vardata[var(cr->first())].reason = nullptr;
    }
    cr->setDeleted();
}

/************************************************************
 * Compute LBD functions
 *************************************************************/
template<class PickBranchLitT>
template <typename Iterator>
inline uint_fast16_t Solver<PickBranchLitT>::computeLBD(Iterator it, Iterator end) {
    uint_fast16_t nblevels = 0;
    MYFLAG++;
    
    for (; it != end; it++) {
        Lit lit = *it;
        if (isSelector(var(lit))) {
            continue;
        }
        int l = trail_abstraction.level(var(lit));
        if (permDiff[l] != MYFLAG) {
            permDiff[l] = MYFLAG;
            nblevels++;
        }
    }
    
    return nblevels;
}

/******************************************************************
 * Minimisation with binary clauses of the asserting clause
 ******************************************************************/
template <class PickBranchLitT>
bool Solver<PickBranchLitT>::minimisationWithBinaryResolution(vector<Lit>& out_learnt) {
    MYFLAG++;

    for (auto it = out_learnt.begin()+1; it != out_learnt.end(); it++) {
        permDiff[var(*it)] = MYFLAG;
    }
    
    bool minimize = false;
    for (Watcher w : watches[0][~out_learnt[0]]) {
        if (permDiff[var(w.blocker)] == MYFLAG && trail_abstraction.value(w.blocker) == l_True) {
            minimize = true;
            permDiff[var(w.blocker)] = MYFLAG - 1;
        }
    }

    if (minimize) {
        auto end = std::remove_if(out_learnt.begin()+1, out_learnt.end(), [this] (Lit lit) { return permDiff[var(lit)] != MYFLAG; } );
        Statistics::getInstance().solverReducedClausesInc(std::distance(end, out_learnt.end()));
        out_learnt.erase(end, out_learnt.end());
    }

    return minimize;
}

/**************************************************************************************************
 *
 *  analyze : (confl : Clause*) (out_learnt : vector<Lit>&) (out_btlevel : int&)  ->  [void]
 *
 *  Description:
 *    Analyze conflict and produce a reason clause.
 *
 *    Pre-conditions:
 *      - 'out_learnt' is assumed to be cleared.
 *      - Current decision level must be greater than root level.
 *
 *    Post-conditions:
 *      - 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
 *      - If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
 *        rest of literals. There may be others from the same level though.
 *
 ***************************************************************************************************/
template <class PickBranchLitT>
void Solver<PickBranchLitT>::analyze(Clause* confl, vector<Lit>& out_learnt, uint_fast16_t &lbd) {
    int pathC = 0;
    Lit asslit = lit_Undef;
    vector<Lit> selectors;
    vector<Var> lastDecisionLevel;
    
    // Generate conflict clause:
    out_learnt.push_back(lit_Undef); // (leave room for the asserting literal)
    int index = trail_abstraction.trail_size - 1;
    do {
        assert(confl != nullptr); // (otherwise should be UIP)
        Clause& c = *confl;
        
        claBumpActivity(c);

        // Special case for binary clauses: The first one has to be SAT
        if (asslit != lit_Undef && c.size() == 2 && trail_abstraction.value(c[0]) == l_False) {
            assert(trail_abstraction.value(c[1]) == l_True);
            c.swap(0, 1);
        }
        
        // DYNAMIC NBLEVEL trick (see competition'09 companion paper)
        if (c.isLearnt() && c.getLBD() > 2) {
            uint_fast16_t nblevels = computeLBD(c.begin(), c.end());
            if (nblevels + 1 < c.getLBD()) { // improve the LBD
                if (c.getLBD() <= lbLBDFrozenClause) {
                    // seems to be interesting : keep it for the next round
                    c.setFrozen(true);
                }
                c.setLBD(nblevels);
            }
        }
        
        for (auto it = (asslit == lit_Undef) ? c.begin() : c.begin() + 1; it != c.end(); it++) {
            Var v = var(*it);
            if (!seen[v] && trail_abstraction.level(v) != 0) {
                seen[v] = 1;
                if (trail_abstraction.level(v) >= (int)trail_abstraction.decisionLevel()) {
                    pathC++;
                    if (trail_abstraction.reason(v) != nullptr && trail_abstraction.reason(v)->isLearnt()) {
                        lastDecisionLevel.push_back(v);
                    }
                } else {
                    if (isSelector(v)) {
                        assert(trail_abstraction.value(*it) == l_False);
                        selectors.push_back(*it);
                    } else {
                        out_learnt.push_back(*it);
                    }
                }
                varBumpActivity(v);
            }
        }
        
        // Select next clause to look at:
        while (!seen[var(trail_abstraction.trail[index])]) {
            index--;
        }
        
        asslit = trail_abstraction.trail[index];
        seen[var(asslit)] = 0;
        confl = trail_abstraction.reason(var(asslit));
        pathC--;
    } while (pathC > 0);
    
    out_learnt[0] = ~asslit;
    
    // Simplify conflict clause:
    out_learnt.insert(out_learnt.end(), selectors.begin(), selectors.end());
    Statistics::getInstance().solverMaxLiteralsInc(checked_unsigned_cast<std::remove_reference<decltype(out_learnt)>::type::size_type, unsigned int>(out_learnt.size()));

    analyze_clear.clear();
    for_each(out_learnt.begin(), out_learnt.end(), [this] (Lit lit) { analyze_clear.push_back(var(lit)); });

    // minimize clause
    uint64_t abstract_level = 0;
    for (uint_fast16_t i = 1; i < out_learnt.size(); i++) {
        abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)
    }
    auto end = remove_if(out_learnt.begin()+1, out_learnt.end(),
                         [this, abstract_level] (Lit lit) {
                             return trail_abstraction.reason(var(lit)) != nullptr && litRedundant(lit, abstract_level);
                         });
    out_learnt.erase(end, out_learnt.end());

    // clear seen[]
    for_each(analyze_clear.begin(), analyze_clear.end(), [this] (Var v) { seen[v] = 0; });
    
    assert(out_learnt[0] == ~asslit);

    lbd = computeLBD(out_learnt.begin(), out_learnt.end() - selectors.size());
    if (lbd <= lbLBDMinimizingClause && out_learnt.size() <= lbSizeMinimizingClause) {
        bool minimized = minimisationWithBinaryResolution(out_learnt);
        if (minimized) {
            lbd = computeLBD(out_learnt.begin(), out_learnt.end());
        }
    }
    
    Statistics::getInstance().solverTotLiteralsInc(checked_unsigned_cast<std::remove_reference<decltype(out_learnt)>::type::size_type, unsigned int>(out_learnt.size()));

    // UPDATEVARACTIVITY trick (see competition'09 companion paper)
    for (Var v : lastDecisionLevel) {
        if (trail_abstraction.reason(v)->getLBD() < lbd) {
            varBumpActivity(v);
        }
    }
}

// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
template <class PickBranchLitT>
bool Solver<PickBranchLitT>::litRedundant(Lit lit, uint64_t abstract_levels) {
    size_t top = analyze_clear.size();

    analyze_stack.clear();
    analyze_stack.push_back(var(lit));

    while (analyze_stack.size() > 0) {
        assert(trail_abstraction.reason(analyze_stack.back()) != nullptr);

        Clause* clause = trail_abstraction.reason(analyze_stack.back());
        analyze_stack.pop_back();

        if (clause->size() == 2 && trail_abstraction.value(clause->first()) == l_False) {
            assert(trail_abstraction.value(clause->second()) == l_True);
            clause->swap(0, 1);
        }

        for (Lit imp : *clause) {
            Var v = var(imp);
            if (!seen[v] && trail_abstraction.level(v) > 0) {
                if (trail_abstraction.reason(v) != nullptr && (abstractLevel(v) & abstract_levels) != 0) {
                    seen[v] = 1;
                    analyze_stack.push_back(v);
                    analyze_clear.push_back(v);
                } else {
                    auto begin = analyze_clear.begin() + top;
                    for_each(begin, analyze_clear.end(), [this](Var v) { seen[v] = 0; });
                    analyze_clear.erase(begin, analyze_clear.end());
                    return false;
                }
            }
        }
    }
    
    return true;
}

/**************************************************************************************************
 *
 *  analyzeFinal : (p : Lit)  ->  [void]
 *
 *  Description:
 *    Specialized analysis procedure to express the final conflict in terms of assumptions.
 *    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
 *    stores the result in 'out_conflict'.
 |*************************************************************************************************/
template <class PickBranchLitT>
void Solver<PickBranchLitT>::analyzeFinal(Lit p, vector<Lit>& out_conflict) {
    out_conflict.clear();
    out_conflict.push_back(p);
    
    if (trail_abstraction.decisionLevel() == 0)
        return;
    
    seen[var(p)] = 1;
    
    for (int i = trail_abstraction.trail_size - 1; i >= (int)trail_abstraction.trail_lim[0]; i--) {
        Var x = var(trail_abstraction.trail[i]);
        if (seen[x]) {
            if (trail_abstraction.reason(x) == nullptr) {
                assert(trail_abstraction.level(x) > 0);
                out_conflict.push_back(~trail_abstraction.trail[i]);
            } else {
                Clause* c = trail_abstraction.reason(x);
                for (Lit lit : *c) {
                    if (trail_abstraction.level(var(lit)) > 0) {
                        seen[var(lit)] = 1;
                    }
                }
            }
            seen[x] = 0;
        }
    }
    seen[var(p)] = 0;
}

// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
template <class PickBranchLitT>
void Solver<PickBranchLitT>::cancelUntil(int level) {
    if ((int)trail_abstraction.decisionLevel() > level) {
        for (int c = trail_abstraction.trail_size - 1; c >= (int)trail_abstraction.trail_lim[level]; c--) {
            Var x = var(trail_abstraction.trail[c]);
            trail_abstraction.assigns[x] = l_Undef;
            polarity[x] = sign(trail_abstraction.trail[c]);
            insertVarOrder(x);
        }
        trail_abstraction.qhead = trail_abstraction.trail_lim[level];
        trail_abstraction.trail_size = trail_abstraction.trail_lim[level];
        trail_abstraction.trail_lim.erase(trail_abstraction.trail_lim.begin() + level, trail_abstraction.trail_lim.end());
    }
}

/**************************************************************************************************
 *
 *  propagate : [void]  ->  [Clause*]
 *
 *  Description:
 *    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
 *    otherwise CRef_Undef.
 *
 *    Post-conditions:
 *      * the propagation queue is empty, even if there was a conflict.
 **************************************************************************************************/
template <class PickBranchLitT>
Clause* Solver<PickBranchLitT>::future_propagate_clauses(Lit p, uint_fast8_t n) {
    assert(n < watches.size());

    vector<Watcher>& list = watches[n][p];

    if (n == 0) { // propagate binary clauses
        for (Watcher& watcher : list) {
            lbool val = trail_abstraction.value(watcher.blocker);
            if (val == l_False) {
                return watcher.cref;
            }
            if (val == l_Undef) {
                trail_abstraction.uncheckedEnqueue(watcher.blocker, watcher.cref);
            }
        }
    }
    else { // propagate other clauses
        auto keep = list.begin();
        for (auto watcher = list.begin(); watcher != list.end(); watcher++) {
            lbool val = trail_abstraction.value(watcher->blocker);
            if (val != l_True) { // Try to avoid inspecting the clause
                Clause* clause = watcher->cref;

                if (clause->first() == ~p) { // Make sure the false literal is data[1]
                    clause->swap(0, 1);
                }

                if (watcher->blocker != clause->first()) {
                    watcher->blocker = clause->first(); // repair blocker (why?)
                    val = trail_abstraction.value(clause->first());
                }

                if (val != l_True) {
                    for (uint_fast16_t k = 2; k < clause->size(); k++) {
                        if (trail_abstraction.value((*clause)[k]) != l_False) {
                            clause->swap(1, k);
                            watches[n][~clause->second()].emplace_back(clause, clause->first());
                            goto propagate_skip;
                        }
                    }

                    // did not find watch
                    if (val == l_False) { // conflict
                        list.erase(keep, watcher);
                        return clause;
                    }
                    else { // unit
                        trail_abstraction.uncheckedEnqueue(clause->first(), clause);
                    }
                }
            }
            *keep = *watcher;
            keep++;
            propagate_skip:;
        }
        list.erase(keep, list.end());
    }

    return nullptr;
}

#ifndef FUTURE_PROPAGATE
template <class PickBranchLitT>
Clause* Solver<PickBranchLitT>::propagate() {
    while (qhead < trail_size) {
        Lit p = trail[qhead++];

        // Propagate binary clauses
        Clause* conflict = future_propagate_clauses(p, 0);
        if (conflict != nullptr) return conflict;

        // Propagate other 2-watched clauses
        conflict = future_propagate_clauses(p, 1);
        if (conflict != nullptr) return conflict;
    }

    return nullptr;
}

#else // FUTURE PROPAGATE

template <class PickBranchLitT>
Clause* Solver<PickBranchLitT>::propagate() {
    std::array<uint32_t, NWATCHES> pos;
    pos.fill(trail_abstraction.qhead);

    while (pos[0] < trail_abstraction.trail_size) {
        for (unsigned int i = 0; i < pos.size(); i++) {
            if (i > 0 && i < 5) allocator.prefetchPage(i + 2);
            while (pos[i] < trail_abstraction.trail_size) {
                Lit p = trail_abstraction.trail[pos[i]++];
                Clause* conflict = future_propagate_clauses(p, i);
                if (conflict != nullptr) return conflict;
            }
        }
    }

    trail_abstraction.qhead = pos[0];

    return nullptr;
}

#endif

/**************************************************************************************************
 *
 *  reduceDB : ()  ->  [void]
 *
 *  Description:
 *    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
 *    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
 **************************************************************************************************/
template <class PickBranchLitT>
void Solver<PickBranchLitT>::reduceDB() {
    Statistics::getInstance().solverReduceDBInc();
    std::sort(learnts.begin(), learnts.end(), reduceDB_lt());
    
    size_t index = (learnts.size() + persist.size()) / 2;
    if (index >= learnts.size() || learnts[index]->getLBD() <= 3) {
        return; // We have a lot of "good" clauses, it is difficult to compare them, keep more
    }
    
    // Delete clauses from the first half which are not locked. (Binary clauses are kept separately and are not touched here)
    // Keep clauses which seem to be useful (i.e. their lbd was reduce during this sequence => frozen)
    size_t limit = std::min(learnts.size(), index);
    for (size_t i = 0; i < limit; i++) {
        Clause* c = learnts[i];
        if (c->getLBD() <= persistentLBD) break; // small lbds come last in sequence (see ordering by reduceDB_lt())
        if (!c->isFrozen() && (trail_abstraction.value(c->first()) != l_True || trail_abstraction.reason(var(c->first())) != c)) {//&& !locked(c)) {
            removeClause(c);
        }
    }

    for (auto& watchers : watches) {
        watchers.cleanAll();
    }
    size_t old_size = learnts.size();
    freeMarkedClauses(learnts);
    Statistics::getInstance().solverRemovedClausesInc(old_size - learnts.size());
    for (Clause* c : learnts) { // "unfreeze" remaining clauses
        c->setFrozen(false);
    }
}

/**
 * Make sure all references are cleared before space is handed back to ClauseAllocator
 */
template <class PickBranchLitT>
void Solver<PickBranchLitT>::freeMarkedClauses(vector<Clause*>& list) {
    auto new_end = std::remove_if(list.begin(), list.end(),
                                  [this](Clause* c) {
                                      if (c->isDeleted()) {
                                          this->allocator.deallocate(c);
                                          return true;
                                      } else {
                                          return false;
                                      }
                                  });
    list.erase(new_end, list.end());
}

template <class PickBranchLitT>
void Solver<PickBranchLitT>::rebuildOrderHeap() {
    vector<Var> vs;
    for (size_t v = 0; v < nVars(); v++) {
        if (decision[v] && trail_abstraction.value(checked_unsignedtosigned_cast<size_t, Var>(v)) == l_Undef) {
            vs.push_back(checked_unsignedtosigned_cast<size_t, Var>(v));
        }
    }
    order_heap.build(vs);
}

template <class PickBranchLitT>
void Solver<PickBranchLitT>::revampClausePool(uint_fast8_t upper) {
    assert(upper <= REVAMPABLE_PAGES_MAX_SIZE);
    
    size_t old_clauses_size = clauses.size();
    size_t old_learnts_size = learnts.size();
    
    // save trail of unary propagations
    vector<Lit> props(trail_abstraction.trail.begin(), trail_abstraction.trail.begin() + trail_abstraction.trail_size);
    for (Lit p : props) {
        trail_abstraction.assigns[var(p)] = l_Undef;
        trail_abstraction.vardata[var(p)] = VarData(nullptr, 0);
    }
    trail_abstraction.trail_size = 0;

    // detach and remove clauses
    for (auto list : { &clauses, &learnts }) {
        for (Clause* c : *list) {
            if (c->size() > 2 && c->size() <= upper) {
                assert(!trail_abstraction.locked(c)); assert(!c->isDeleted());
                detachClause(c, true);
            }
        }
        list->erase(std::remove_if(list->begin(), list->end(), [this,upper](Clause* c) {
                return (c->size() > 2 && c->size() <= upper); } ), list->end());
    }
    
    // revamp and re-attach clauses
    for (uint_fast8_t k = 3; k <= upper; k++) {
        vector<Clause*> revamped = allocator.revampPages(k);
        for (Clause* clause : revamped) {
            attachClause(clause);
            if (clause->isLearnt()) {
                learnts.push_back(clause);
            } else {
                clauses.push_back(clause);
            }
        }
    }
    
    // cleanup
    for (auto& watchers : watches) {
        watchers.cleanAll();
    }

    // restore trail of unary propagation
    for (Lit p : props) {
        if (trail_abstraction.assigns[var(p)] == l_Undef) {
            trail_abstraction.uncheckedEnqueue(p);
            propagate();
        }
    }
    
    assert(old_learnts_size == learnts.size());
    assert(old_clauses_size == clauses.size());
    (void)(old_learnts_size);
    (void)(old_clauses_size);
}

/**************************************************************************************************
 *
 *  simplify : [void]  ->  [bool]
 *
 *  Description:
 *    Simplify the clause database according to the current top-level assignment. Currently, the only
 *    thing done here is the removal of satisfied clauses, but more things can be put here.
 **************************************************************************************************/
template <class PickBranchLitT>
bool Solver<PickBranchLitT>::simplify() {
    assert(trail_abstraction.decisionLevel() == 0);
    
    if (!ok || propagate() != nullptr) {
        return ok = false;
    }

    // Remove satisfied clauses:
    std::for_each(learnts.begin(), learnts.end(), [this] (Clause* c) { if (trail_abstraction.satisfied(*c)) removeClause(c); } );
    std::for_each(persist.begin(), persist.end(), [this] (Clause* c) { if (trail_abstraction.satisfied(*c)) removeClause(c); } );
    std::for_each(clauses.begin(), clauses.end(), [this] (Clause* c) { if (trail_abstraction.satisfied(*c)) removeClause(c); });

    // Cleanup:
    for (auto& watchers : watches) {
        watchers.cleanAll();
    }

    freeMarkedClauses(learnts);
    freeMarkedClauses(persist);
    freeMarkedClauses(clauses);

    rebuildOrderHeap();

    return true;
}

template <class PickBranchLitT>
bool Solver<PickBranchLitT>::strengthen() {
    // Remove false literals:
    std::unordered_map<Clause*, size_t> strengthened_sizes;
    std::vector<Clause*> strengthened_clauses;
    for (auto list : { clauses, learnts, persist }) {
        for (Clause* clause : list) if (!clause->isDeleted()) {
            auto pos = find_if(clause->begin(), clause->end(), [this] (Lit lit) { return trail_abstraction.value(lit) == l_False; });
            if (pos != clause->end()) {
                detachClause(clause);
                certificate->removed(clause->begin(), clause->end());
                auto end = remove_if(clause->begin(), clause->end(), [this] (Lit lit) { return trail_abstraction.value(lit) == l_False; });
                certificate->added(clause->begin(), end);
                strengthened_clauses.push_back(clause);
                strengthened_sizes[clause] = clause->size();
                clause->setSize(std::distance(clause->begin(), end));
            }
        }
    }

    allocator.announceClauses(strengthened_clauses);
    for (Clause* clause : strengthened_clauses) {
        if (clause->size() == 1) {
            Lit p = clause->first();
            if (trail_abstraction.value(p) == l_True) {
                trail_abstraction.vardata[var(p)].reason = nullptr;
                trail_abstraction.vardata[var(p)].level = 0;
            }
            else {
                trail_abstraction.uncheckedEnqueue(p);
            }
        }
        else {
            Clause* clean = new (allocator.allocate(clause->size())) Clause(*clause);
            if (trail_abstraction.locked(clause)) {
                trail_abstraction.vardata[var(clause->first())].reason = clean;
            }
            attachClause(clean);
            if (clean->isLearnt()) {
                if (clean->size() == 2) {
                    persist.push_back(clean);
                } else {
                    learnts.push_back(clean);
                }
            } else {
                clauses.push_back(clean);
            }
        }
        clause->setSize(strengthened_sizes[clause]); // restore original size for freeMarkedClauses
        clause->setDeleted();
    }

    // Cleanup:
    for (auto& watchers : watches) {
        watchers.cleanAll();
    }

    freeMarkedClauses(learnts);
    freeMarkedClauses(persist);
    freeMarkedClauses(clauses);

    rebuildOrderHeap();

    return ok = (propagate() == nullptr);
}

/**
 * Dummy virtual method for inprocessing
 */
template<class PickBranchLitT>
bool Solver<PickBranchLitT>::eliminate(bool use_asymm, bool use_elim) {
    return true;
}

/**************************************************************************************************
 *
 *  search : (nof*conflicts : int) (params : const SearchParams&)  ->  [lbool]
 *
 *  Description:
 *    Search for a model the specified number of conflicts.
 *    NOTE! Use negative value for 'nof*conflicts' indicate infinity.
 *
 *  Output:
 *    'l*True' if a partial assigment that is consistent with respect to the clauseset is found. If
 *    all variables are decision variables, this means that the clause set is satisfiable. 'l*False'
 *    if the clause set is unsatisfiable. 'l*Undef' if the bound on number of conflicts is reached.
 **************************************************************************************************/
template <class PickBranchLitT>
lbool Solver<PickBranchLitT>::search() {
    assert(ok);
    
    vector<Lit> learnt_clause;
    uint_fast16_t nblevels;
    bool blocked = false;
    bool reduced = false;
    Statistics::getInstance().solverRestartInc();
    sonification.restart();
    for (;;) {
        sonification.decisionLevel(trail_abstraction.decisionLevel(), SolverOptions::opt_sonification_delay);
        

        uint32_t old_qhead = trail_abstraction.qhead;

        Clause* confl = propagate();

        nPropagations += trail_abstraction.qhead - old_qhead;
        
        sonification.assignmentLevel(static_cast<int>(trail_abstraction.nAssigns()));
        
        if (confl != nullptr) { // CONFLICT
            sonification.conflictLevel(trail_abstraction.decisionLevel());
            
            ++nConflicts;
            
            if (nConflicts % 5000 == 0 && var_decay < max_var_decay) {
                var_decay += 0.01;
            }
            
            if (verbosity >= 1 && nConflicts % verbEveryConflicts == 0) {
                Statistics::getInstance().printIntermediateStats((int) (trail_abstraction.trail_lim.size() == 0 ? trail_abstraction.trail_size : trail_abstraction.trail_lim[0]),
                    static_cast<int>(nClauses()),
                    static_cast<int>(nLearnts()),
                    static_cast<int>(nConflicts));
            }
            if (trail_abstraction.decisionLevel() == 0) {
                return l_False;
            }
            
            trailQueue.push(trail_abstraction.trail_size);
            
            // BLOCK RESTART (CP 2012 paper)
            if (nConflicts > 10000 && lbdQueue.isvalid() && trail_abstraction.trail_size > R * trailQueue.getavg()) {
                lbdQueue.fastclear();
                Statistics::getInstance().solverStopsRestartsInc();
                if (!blocked) {
                    Statistics::getInstance().solverLastBlockAtRestartSave();
                    Statistics::getInstance().solverStopsRestartsSameInc();
                    blocked = true;
                }
            }
            
            learnt_clause.clear();
            
            analyze(confl, learnt_clause, nblevels);
            
            if (learntCallback != nullptr && (int)learnt_clause.size() <= learntCallbackMaxLength) {
                vector<int> clause(learnt_clause.size() + 1);
                for (Lit lit : learnt_clause) {
                    clause.push_back((var(lit)+1)*(sign(lit)?-1:1));
                }
                clause.push_back(0);
                learntCallback(learntCallbackState, &clause[0]);
            }

            sonification.learntSize(static_cast<int>(learnt_clause.size()));

            if (nblevels <= 2) {
                Statistics::getInstance().solverLBD2Inc();
            }
            if (learnt_clause.size() == 1) {
                Statistics::getInstance().solverUnariesInc();
            }
            if (learnt_clause.size() == 2) {
                Statistics::getInstance().solverBinariesInc();
            }

            if (!isSelector(learnt_clause.back())) {
                certificate->added(learnt_clause.begin(), learnt_clause.end());
            }

            lbdQueue.push(nblevels);
            sumLBD += nblevels;

            if (learnt_clause.size() == 1) {
                cancelUntil(0);
                trail_abstraction.uncheckedEnqueue(learnt_clause[0]);
                new_unary = true;
            }
            else {
                uint32_t clauseLength = checked_unsigned_cast<decltype(learnt_clause)::size_type, uint32_t>(learnt_clause.size());
                Clause* cr = new (allocator.allocate(clauseLength)) Clause(learnt_clause, nblevels);

                if (clauseLength == 2) {
                    persist.push_back(cr);
                }
                else {
                    learnts.push_back(cr);
                    // Find correct backtrack level:
                    int max_i = 1;
                    // Find the first literal assigned at the next-highest level:
                    for (uint_fast16_t i = 2; i < cr->size(); i++)
                        if (trail_abstraction.level(var((*cr)[i])) > trail_abstraction.level(var((*cr)[max_i])))
                            max_i = i;
                    // Swap-in this literal at index 1:
                    cr->swap(max_i, 1);
                }

                cancelUntil(trail_abstraction.level(var(cr->second())));

                attachClause(cr);
                trail_abstraction.uncheckedEnqueue(cr->first(), cr);
                claBumpActivity(*cr);
            }
            claDecayActivity();
            varDecayActivity();
        }
        else {
            // Our dynamic restart, see the SAT09 competition compagnion paper
            if ((lbdQueue.isvalid() && ((lbdQueue.getavg() * K) > (sumLBD / nConflicts)))) {
                lbdQueue.fastclear();
                
                cancelUntil(0);
                
                // every restart after reduce-db
                if (reduced) {
                    reduced = false;

                    if (inprocessingFrequency > 0 && lastRestartWithInprocessing + inprocessingFrequency <= curRestart) {
                        lastRestartWithInprocessing = curRestart;
                        Statistics::getInstance().runtimeStart("Inprocessing");
                        if (!eliminate(false, false)) {
                            return l_False;
                        }
                        Statistics::getInstance().runtimeStop("Inprocessing");
                    }
                    else if (new_unary) {
                        new_unary = false;
                        Statistics::getInstance().runtimeStart("Simplify");
                        if (!simplify()) {
                            return l_False;
                        }
                        Statistics::getInstance().runtimeStop("Simplify");
                    }

                    if (revamp > 2) {
                        Statistics::getInstance().runtimeStart("Revamp");
                        revampClausePool(revamp);
                        Statistics::getInstance().runtimeStop("Revamp");
                    }
                    
                    if (sort_watches) {
                        Statistics::getInstance().runtimeStart("Sort Watches");
                        for (size_t v = 0; v < nVars(); v++) {
                            Var vVar = checked_unsignedtosigned_cast<size_t, Var>(v);
                            for (Lit l : { mkLit(vVar, false), mkLit(vVar, true) }) {
                                for (size_t i = 1; i < watches.size()-1; i++) {
                                    sort(watches[i][l].begin(), watches[i][l].end(), [](Watcher w1, Watcher w2) {
                                        Clause& c1 = *w1.cref;
                                        Clause& c2 = *w2.cref;
                                        return c1.activity() > c2.activity();
                                    });
                                }
                                sort(watches.back()[l].begin(), watches.back()[l].end(), [](Watcher w1, Watcher w2) {
                                    Clause& c1 = *w1.cref;
                                    Clause& c2 = *w2.cref;
                                    return c1.size() < c2.size() || (c1.size() == c2.size() && c1.activity() > c2.activity());
                                });
                            }
                        }
                        Statistics::getInstance().runtimeStop("Sort Watches");
                    }
                }
                
                return l_Undef;
            }
            
            // Perform clause database reduction !
            if (nConflicts >= (curRestart * nbclausesbeforereduce)) {
                if (learnts.size() > 0) {
                    curRestart = (nConflicts / nbclausesbeforereduce) + 1;
                    reduceDB();
                    reduced = true;
                    nbclausesbeforereduce += incReduceDB;
                }
            }
            
            Lit next = lit_Undef;
            while (trail_abstraction.decisionLevel() < assumptions.size()) {
                // Perform user provided assumption:
                Lit p = assumptions[trail_abstraction.decisionLevel()];
                if (trail_abstraction.value(p) == l_True) {
                    // Dummy decision level:
                    trail_abstraction.newDecisionLevel();
                } else if (trail_abstraction.value(p) == l_False) {
                    analyzeFinal(~p, conflict);
                    return l_False;
                } else {
                    next = p;
                    break;
                }
            }
            
            if (next == lit_Undef) {
                // New variable decision:
                Statistics::getInstance().solverDecisionsInc();
                next = pickBranchLit();
                if (next == lit_Undef) {
                    // Model found:
                    return l_True;
                }
            }
            
            // Increase decision level and enqueue 'next'
            trail_abstraction.newDecisionLevel();
            trail_abstraction.uncheckedEnqueue(next);
        }
    }
    return l_Undef; // not reached
}

// NOTE: assumptions passed in member-variable 'assumptions'.
// Parameters are useless in core but useful for SimpSolver....
template <class PickBranchLitT>
lbool Solver<PickBranchLitT>::solve() {
    Statistics::getInstance().runtimeStart("Solver");

    sonification.start(static_cast<int>(nVars()), static_cast<int>(nClauses()));
    
    model.clear();
    conflict.clear();
    
    if (!incremental && verbosity >= 1) {
        printf("c =====================================[ MAGIC CONSTANTS ]======================================\n");
        printf("c | Constants are supposed to work well together :-)                                           |\n");
        printf("c | however, if you find better choices, please let us known...                                |\n");
        printf("c |--------------------------------------------------------------------------------------------|\n");
        printf("c |                                |                                |                          |\n");
        printf("c | - Restarts:                    | - Reduce Clause DB:            | - Minimize Asserting:    |\n");
        printf("c |   * LBD Queue    : %6d      |   * First     : %6d         |    * size < %3d          |\n", lbdQueue.maxSize(), nbclausesbeforereduce, lbSizeMinimizingClause);
        printf("c |   * Trail  Queue : %6d      |   * Inc       : %6d         |    * lbd  < %3d          |\n", trailQueue.maxSize(), incReduceDB, lbLBDMinimizingClause);
        printf("c |   * K            : %6.2f      |   * Persistent: %6d         |                          |\n", K, persistentLBD);
        printf("c |   * R            : %6.2f      |   * Protected :  (lbd)< %2d     |                          |\n", R, lbLBDFrozenClause);
        printf("c |                                |                                |                          |\n");
        printf("c =========================[ Search Statistics (every %6d conflicts) ]=======================\n", verbEveryConflicts);
        printf("c |                                                                                            |\n");
        printf("c |      RESTARTS      |       ORIGINAL      |                     LEARNT                      |\n");
        printf("c |  NB  Blocked  Avg  |  Vars    Clauses    |  Red  Learnts    Binary  Unary  LBD2  Removed   |\n");
        printf("c ==============================================================================================\n");
    }

    lbool status = l_Undef;
    if (isInConflictingState()) {
        status = l_False;
    }
    
    // Search:
    while (status == l_Undef && withinBudget()) {
        status = search();
    }
    
    if (status == l_False) {
        Statistics::getInstance().incNBUnsatCalls();
        
        if (!incremental) {
            certificate->proof();
        }
        else {
            // check if selectors are used in final conflict
            vector<Lit> finalConflict;
            analyzeFinal(trail_abstraction.trail[trail_abstraction.trail_size-1], finalConflict);
            auto pos = find_if(finalConflict.begin(), finalConflict.end(), [this] (Lit lit) { return isSelector(var(lit)); } );
            if (pos == finalConflict.end()) {
                certificate->proof();
            }
        }
        
        if (conflict.size() == 0) {
            ok = false;
        }

        sonification.stop(1);
    }
    else if (status == l_True) {
        Statistics::getInstance().incNBSatCalls();
        
        model.clear();
        model.insert(model.end(), trail_abstraction.assigns.begin(), trail_abstraction.assigns.end());

        sonification.stop(0);
    }
    else {
        sonification.stop(-1);
    }
    
    cancelUntil(0);
    
    Statistics::getInstance().runtimeStop("Solver");
    return status;
}

template <class PickBranchLitT>
ATTR_ALWAYSINLINE
inline Lit Solver<PickBranchLitT>::defaultPickBranchLit() {
    Var next = var_Undef;
    
    // Activity based decision:
    while (next == var_Undef || trail_abstraction.value(next) != l_Undef || !decision[next]) {
        if (order_heap.empty()) {
            next = var_Undef;
            break;
        } else {
            next = order_heap.removeMin();
        }
    }
    
    return next == var_Undef ? lit_Undef : mkLit(next, polarity[next]);
}

}

#endif
