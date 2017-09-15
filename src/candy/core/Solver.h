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

#include "candy/core/Statistics.h"
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
#include "candy/core/Trail.h"
#include "candy/core/Propagate.h"
#include "candy/core/Branch.h"
#include "candy/core/Stamp.h"

#include "candy/sonification/SolverSonification.h"
#include "candy/sonification/ControllerInterface.h"

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
    static_assert(std::is_class<typename PickBranchLitT::Parameters>::value, "PickBranchLitT::Parameters must be a class");
    static_assert(std::is_constructible<PickBranchLitT>::value, "PickBranchLitT must have a constructor without arguments");
    static_assert(std::is_move_assignable<PickBranchLitT>::value, "PickBranchLitT must be move-assignable");
    static_assert(std::is_move_constructible<PickBranchLitT>::value, "PickBranchLitT must be move-constructible");

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

    template<typename Iterator>
    bool addClauseSanitize(Iterator begin, Iterator end);

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
        return trail.vardata.size();
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

    Propagate propagator;
    Trail trail;

    Branch branch;

	vector<Lit> assumptions; // Current set of assumptions provided to solve by the user.

    // clause activity heuristic
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
    Stamp<uint32_t> stamp;

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
    ControllerInterface controller;
    
    // Variables for decisions
    PickBranchLitT pickBranchLitData;

    inline uint64_t abstractLevel(Var x) const {
        return 1ull << (trail.level(x) % 64);
    }

    // Operations on clauses:
    void removeClause(Clause* cr, bool strict_detach = false); // Detach and free a clause.
    void freeMarkedClauses(vector<Clause*>& list);

    template <typename Iterator>
    uint_fast16_t computeLBD(Iterator it, Iterator end);
    void minimisationWithBinaryResolution(vector<Lit> &out_learnt);

    // Return the next decision variable.
    Lit pickBranchLit() {
        return branch.pickBranchLit(trail);
    }

    Lit defaultPickBranchLit(); // Return the next decision variable (default implementation).
    void cancelUntil(int level); // Backtrack until a certain level.
    void conflict_analysis_generate_clause(Clause* confl, vector<Lit>& out_learnt);
    void conflict_anlysis_dynamic_heuristics(Clause* confl, unsigned int learnt_lbd);
    void analyzeFinal(Lit p, vector<Lit>& out_conflict); // COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
    bool litRedundant(Lit p, uint64_t abstract_levels); // (helper method for 'analyze()')
    lbool search(); // Search for a given number of conflicts.
    virtual void reduceDB(); // Reduce the set of learnt clauses.
    void revampClausePool(uint8_t upper);

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
    // propagate
    propagator(),
    // current assignment
    trail(),
	// branching heuristic
	branch(SolverOptions::opt_var_decay, SolverOptions::opt_max_var_decay),
    // assumptions
    assumptions(),
    // clause activity based heuristic
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
    stamp(),
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
	controller(),
    pickBranchLitData()
{
controller.run();
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
    propagator.init(nVars());
    trail.grow();
    seen.push_back(0);
    stamp.incSize();
    branch.grow(dvar, sign, act);
    return v;
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::addClauses(const CNFProblem& dimacs) {
    const std::vector<std::vector<Lit>*>& problem = dimacs.getProblem();
    size_t curVars = this->nVars();
    size_t maxVars = (size_t)dimacs.nVars();
    if (maxVars > curVars) {
        propagator.init(maxVars);
        trail.grow(maxVars);
        seen.resize(maxVars, 0);
        stamp.incSize(maxVars);
        branch.grow(maxVars, true, true, 0.0);
        if (sort_variables) {
        	branch.initFrom(dimacs);
        	branch.rebuildOrderHeap(trail);
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
    assert(trail.decisionLevel() == 0);

    uint32_t size = static_cast<uint32_t>(std::distance(begin, end));

    if (size == 0) {
        return ok = false;
    }
    else if (size == 1) {
        if (trail.value(*begin) == l_Undef) {
            trail.uncheckedEnqueue(*begin);
            return ok = (propagator.propagate(trail) == nullptr);
        }
        else if (trail.value(*begin) == l_True) {
            trail.vardata[var(*begin)].reason = nullptr;
            return ok;
        }
        else {
            return ok = false;
        }
    }
    else {
        Clause* cr = new (allocator.allocate(size)) Clause(begin, end);
        clauses.push_back(cr);
        propagator.attachClause(cr);
    }
    
    return ok;
}

template<class PickBranchLitT>
template<typename Iterator>
bool Solver<PickBranchLitT>::addClauseSanitize(Iterator begin, Iterator end) {
    std::sort(begin, end);
    Iterator uend = std::unique(begin, end);

    return addClause(begin, uend);
}

template<class PickBranchLitT>
void Solver<PickBranchLitT>::removeClause(Clause* cr, bool strict_detach) {
    certificate->removed(cr->begin(), cr->end());
    propagator.detachClause(cr, strict_detach);
    // Don't leave pointers to free'd memory!
    if (trail.locked(cr)) {
        trail.vardata[var(cr->first())].reason = nullptr;
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
    stamp.clearStamped();
    
    for (; it != end; it++) {
        Lit lit = *it;
        if (isSelector(var(lit))) {
            continue;
        }
        int l = trail.level(var(lit));
        if (!stamp.isStamped(l)) {
            stamp.setStamped(l);
            nblevels++;
        }
    }
    
    return nblevels;
}

/******************************************************************
 * Minimisation with binary clauses of the asserting clause
 ******************************************************************/
template <class PickBranchLitT>
void Solver<PickBranchLitT>::minimisationWithBinaryResolution(vector<Lit>& out_learnt) {
    stamp.clearStamped();

    for (auto it = out_learnt.begin()+1; it != out_learnt.end(); it++) {
        stamp.setStamped(var(*it));
    }
    
    bool minimize = false;
    for (Watcher w : propagator.getBinaryWatchers(~out_learnt[0])) {
        if (stamp.isStamped(var(w.blocker)) && trail.value(w.blocker) == l_True) {
            minimize = true;
            stamp.unsetStamped(var(w.blocker));
        }
    }

    if (minimize) {
        auto end = std::remove_if(out_learnt.begin()+1, out_learnt.end(), [this] (Lit lit) { return !stamp.isStamped(var(lit)); } );
        Statistics::getInstance().solverReducedClausesInc(std::distance(end, out_learnt.end()));
        out_learnt.erase(end, out_learnt.end());
    }
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
void Solver<PickBranchLitT>::conflict_analysis_generate_clause(Clause* confl, vector<Lit>& out_learnt) {
    int pathC = 0;
    Lit asslit = lit_Undef;
    vector<Lit> selectors;

    // Generate conflict clause:
    out_learnt.push_back(lit_Undef); // (leave room for the asserting literal)
    Trail::const_reverse_iterator trail_iterator = trail.rbegin();
    do {
        assert(confl != nullptr); // (otherwise should be UIP)

        // Special case for binary clauses: The first one has to be SAT
        if (asslit != lit_Undef && confl->size() == 2 && trail.value(confl->first()) == l_False) {
            assert(trail.value(confl->second()) == l_True);
            confl->swap(0, 1);
        }

        for (auto it = (asslit == lit_Undef) ? confl->begin() : confl->begin() + 1; it != confl->end(); it++) {
            Var v = var(*it);
            if (!seen[v] && trail.level(v) != 0) {
                seen[v] = 1;
                if (trail.level(v) >= (int)trail.decisionLevel()) {
                    pathC++;
                } else {
                    if (isSelector(v)) {
                        assert(trail.value(*it) == l_False);
                        selectors.push_back(*it);
                    } else {
                        out_learnt.push_back(*it);
                    }
                }
            }
        }

        // Select next clause to look at:
        while (!seen[var(*trail_iterator)]) {
            trail_iterator++;
        }

        asslit = *trail_iterator;
        seen[var(*trail_iterator)] = 0;
        confl = trail.reason(var(*trail_iterator));
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
                             return trail.reason(var(lit)) != nullptr && litRedundant(lit, abstract_level);
                         });
    out_learnt.erase(end, out_learnt.end());

    // clear seen[]
    for_each(analyze_clear.begin(), analyze_clear.end(), [this] (Var v) { seen[v] = 0; });

    assert(out_learnt[0] == ~asslit);

    if (out_learnt.size() <= lbSizeMinimizingClause) {
        minimisationWithBinaryResolution(out_learnt);
    }

    Statistics::getInstance().solverTotLiteralsInc(checked_unsigned_cast<std::remove_reference<decltype(out_learnt)>::type::size_type, unsigned int>(out_learnt.size()));
}

template <class PickBranchLitT>
void Solver<PickBranchLitT>::conflict_anlysis_dynamic_heuristics(Clause* confl, unsigned int learnt_lbd) {
    int pathC = 0;
    Lit asslit = lit_Undef;
    Trail::const_reverse_iterator trail_iterator = trail.rbegin();
    analyze_clear.clear();
    do {
        assert(confl != nullptr); // (otherwise should be UIP)
        claBumpActivity(*confl);

        // Special case for binary clauses: The first one has to be SAT
        if (asslit != lit_Undef && confl->size() == 2 && trail.value(confl->first()) == l_False) {
            assert(trail.value(confl->second()) == l_True);
            confl->swap(0, 1);
        }

        // DYNAMIC NBLEVEL trick (see competition'09 companion paper)
        if (confl->isLearnt() && confl->getLBD() > persistentLBD) {
            uint_fast16_t nblevels = computeLBD(confl->begin(), confl->end());
            if (nblevels + 1 < confl->getLBD()) { // improve the LBD
                if (confl->getLBD() <= lbLBDFrozenClause) {
                    // seems to be interesting : keep it for the next round
                    confl->setFrozen(true);
                }
                confl->setLBD(nblevels);
            }
        }

        for (auto it = (asslit == lit_Undef) ? confl->begin() : confl->begin() + 1; it != confl->end(); it++) {
            Var v = var(*it);
            if (!seen[v] && trail.level(v) != 0) {
                seen[v] = 1;
                if (trail.level(v) >= (int)trail.decisionLevel()) {
                    pathC++;
                }
                else {
                	analyze_clear.push_back(v);
                }
    	        if (!isSelector(v)) {
    	        	branch.varBumpActivity(v);
                    if (trail.level(v) >= (int)trail.decisionLevel() && trail.reason(v) != nullptr && trail.reason(v)->isLearnt()) {
                        // UPDATEVARACTIVITY trick (see competition'09 companion paper)
                    	if (trail.reason(v)->getLBD() < learnt_lbd) {
                    		branch.varBumpActivity(v);
                    	}
                    }
    	        }
            }
        }

        // Select next clause to look at:
        while (!seen[var(*trail_iterator)]) {
            trail_iterator++;
        }

        asslit = *trail_iterator;
        seen[var(*trail_iterator)] = 0;
        confl = trail.reason(var(*trail_iterator));
        pathC--;
    } while (pathC > 0);
    for_each(analyze_clear.begin(), analyze_clear.end(), [this] (Var v) { seen[v] = 0; });
//    std::fill(seen.begin(), seen.end(), 0);
}

// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
template <class PickBranchLitT>
bool Solver<PickBranchLitT>::litRedundant(Lit lit, uint64_t abstract_levels) {
    size_t top = analyze_clear.size();

    analyze_stack.clear();
    analyze_stack.push_back(var(lit));

    while (analyze_stack.size() > 0) {
        assert(trail.reason(analyze_stack.back()) != nullptr);

        Clause* clause = trail.reason(analyze_stack.back());
        analyze_stack.pop_back();

        if (clause->size() == 2 && trail.value(clause->first()) == l_False) {
            assert(trail.value(clause->second()) == l_True);
            clause->swap(0, 1);
        }

        for (Lit imp : *clause) {
            Var v = var(imp);
            if (!seen[v] && trail.level(v) > 0) {
                if (trail.reason(v) != nullptr && (abstractLevel(v) & abstract_levels) != 0) {
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
    
    if (trail.decisionLevel() == 0)
        return;
    
    seen[var(p)] = 1;
    
    for (int i = trail.size() - 1; i >= (int)trail.trail_lim[0]; i--) {
        Var x = var(trail[i]);
        if (seen[x]) {
            if (trail.reason(x) == nullptr) {
                assert(trail.level(x) > 0);
                out_conflict.push_back(~trail[i]);
            } else {
                Clause* c = trail.reason(x);
                for (Lit lit : *c) {
                    if (trail.level(var(lit)) > 0) {
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
    vector<Lit> lits = trail.cancelUntil(level);
    std::reverse(lits.begin(), lits.end());
    for (Lit lit : lits) {
    	branch.polarity[var(lit)] = sign(lit);
    	branch.insertVarOrder(var(lit));
    }
}

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
        if (!c->isFrozen() && (trail.value(c->first()) != l_True || trail.reason(var(c->first())) != c)) {//&& !locked(c)) {
            removeClause(c);
        }
    }

    propagator.cleanupWatchers();
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
void Solver<PickBranchLitT>::revampClausePool(uint_fast8_t upper) {
    assert(upper <= REVAMPABLE_PAGES_MAX_SIZE);
    
    size_t old_clauses_size = clauses.size();
    size_t old_learnts_size = learnts.size();
    
    // save trail of unary propagations
    vector<Lit> props(trail.begin(), trail.end());
    for (Lit p : props) {
        trail.assigns[var(p)] = l_Undef;
        trail.vardata[var(p)] = VarData(nullptr, 0);
    }
    trail.trail_size = 0;

    // detach and remove clauses
    for (auto list : { &clauses, &learnts }) {
        for (Clause* c : *list) {
            if (c->size() > 2 && c->size() <= upper) {
                assert(!trail.locked(c)); assert(!c->isDeleted());
                propagator.detachClause(c, true);
            }
        }
        list->erase(std::remove_if(list->begin(), list->end(), [this,upper](Clause* c) {
                return (c->size() > 2 && c->size() <= upper); } ), list->end());
    }
    
    // revamp and re-attach clauses
    for (uint_fast8_t k = 3; k <= upper; k++) {
        vector<Clause*> revamped = allocator.revampPages(k);
        for (Clause* clause : revamped) {
            propagator.attachClause(clause);
            if (clause->isLearnt()) {
                learnts.push_back(clause);
            } else {
                clauses.push_back(clause);
            }
        }
    }
    
    // cleanup
    propagator.cleanupWatchers();

    // restore trail of unary propagation
    for (Lit p : props) {
        if (trail.assigns[var(p)] == l_Undef) {
            trail.uncheckedEnqueue(p);
            propagator.propagate(trail);
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
    assert(trail.decisionLevel() == 0);
    
    if (!ok || propagator.propagate(trail) != nullptr) {
        return ok = false;
    }

    // Remove satisfied clauses:
    std::for_each(learnts.begin(), learnts.end(), [this] (Clause* c) { if (trail.satisfied(*c)) removeClause(c); } );
    std::for_each(persist.begin(), persist.end(), [this] (Clause* c) { if (trail.satisfied(*c)) removeClause(c); } );
    std::for_each(clauses.begin(), clauses.end(), [this] (Clause* c) { if (trail.satisfied(*c)) removeClause(c); });

    // Cleanup:
    propagator.cleanupWatchers();

    freeMarkedClauses(learnts);
    freeMarkedClauses(persist);
    freeMarkedClauses(clauses);

    return true;
}

template <class PickBranchLitT>
bool Solver<PickBranchLitT>::strengthen() {
    // Remove false literals:
    std::unordered_map<Clause*, size_t> strengthened_sizes;
    std::vector<Clause*> strengthened_clauses;
    for (auto& list : { clauses, learnts, persist }) {
        for (Clause* clause : list) if (!clause->isDeleted()) {
            auto pos = find_if(clause->begin(), clause->end(), [this] (Lit lit) { return trail.value(lit) == l_False; });
            if (pos != clause->end()) {
                propagator.detachClause(clause);
                certificate->removed(clause->begin(), clause->end());
                auto end = remove_if(clause->begin(), clause->end(), [this] (Lit lit) { return trail.value(lit) == l_False; });
                certificate->added(clause->begin(), end);
                strengthened_clauses.push_back(clause);
                strengthened_sizes[clause] = clause->size();
                clause->setSize(std::distance(clause->begin(), end));
            }
        }
    }

    allocator.announceClauses(strengthened_clauses);
    for (Clause* clause : strengthened_clauses) {
        if (clause->size() == 0) {
            ok = false;
        }
        else if (clause->size() == 1) {
            Lit p = clause->first();
            if (trail.value(p) == l_True) {
                trail.vardata[var(p)].reason = nullptr;
                trail.vardata[var(p)].level = 0;
            }
            else {
                trail.uncheckedEnqueue(p);
            }
        }
        else {
            Clause* clean = new (allocator.allocate(clause->size())) Clause(*clause);
            if (trail.locked(clause)) {
                trail.vardata[var(clause->first())].reason = clean;
            }
            propagator.attachClause(clean);
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
    propagator.cleanupWatchers();

    freeMarkedClauses(learnts);
    freeMarkedClauses(persist);
    freeMarkedClauses(clauses);

    return ok &= (propagator.propagate(trail) == nullptr);
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
        sonification.decisionLevel(trail.decisionLevel(), SolverOptions::opt_sonification_delay);
        

        uint32_t old_qhead = trail.qhead;

        Clause* confl = propagator.propagate(trail);

        nPropagations += trail.qhead - old_qhead;
        
        sonification.assignmentLevel(static_cast<int>(trail.size()));
        
        if (confl != nullptr) { // CONFLICT
            sonification.conflictLevel(trail.decisionLevel());
            
            ++nConflicts;
            
            if (nConflicts % 5000 == 0 && branch.var_decay < branch.max_var_decay) {
            	branch.var_decay += 0.01;
            }
            
            if (verbosity >= 1 && nConflicts % verbEveryConflicts == 0) {
                Statistics::getInstance().printIntermediateStats((int) (trail.trail_lim.size() == 0 ? trail.size() : trail.trail_lim[0]),
                    static_cast<int>(nClauses()),
                    static_cast<int>(nLearnts()),
                    static_cast<int>(nConflicts));
            }
            if (trail.decisionLevel() == 0) {
                return l_False;
            }
            
            trailQueue.push(trail.size());
            
            // BLOCK RESTART (CP 2012 paper)
            if (nConflicts > 10000 && lbdQueue.isvalid() && trail.size() > R * trailQueue.getavg()) {
                lbdQueue.fastclear();
                Statistics::getInstance().solverStopsRestartsInc();
                if (!blocked) {
                    Statistics::getInstance().solverLastBlockAtRestartSave();
                    Statistics::getInstance().solverStopsRestartsSameInc();
                    blocked = true;
                }
            }
            
            learnt_clause.clear();
            
            conflict_analysis_generate_clause(confl, learnt_clause);
            
            if (learntCallback != nullptr && (int)learnt_clause.size() <= learntCallbackMaxLength) {
                vector<int> clause(learnt_clause.size() + 1);
                for (Lit lit : learnt_clause) {
                    clause.push_back((var(lit)+1)*(sign(lit)?-1:1));
                }
                clause.push_back(0);
                learntCallback(learntCallbackState, &clause[0]);
            }

            sonification.learntSize(static_cast<int>(learnt_clause.size()));

            if (learnt_clause.size() == 1) {
                Statistics::getInstance().solverUnariesInc();
            }
            if (learnt_clause.size() == 2) {
                Statistics::getInstance().solverBinariesInc();
            }

            if (!isSelector(learnt_clause.back())) {
                certificate->added(learnt_clause.begin(), learnt_clause.end());
            }

            nblevels = computeLBD(learnt_clause.begin(), learnt_clause.end());

            conflict_anlysis_dynamic_heuristics(confl, nblevels);

            if (nblevels <= 2) {
                Statistics::getInstance().solverLBD2Inc();
            }

            lbdQueue.push(nblevels);
            sumLBD += nblevels;

            if (learnt_clause.size() == 1) {
                cancelUntil(0);
                trail.uncheckedEnqueue(learnt_clause[0]);
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
                        if (trail.level(var((*cr)[i])) > trail.level(var((*cr)[max_i])))
                            max_i = i;
                    // Swap-in this literal at index 1:
                    cr->swap(max_i, 1);
                }

                cancelUntil(trail.level(var(cr->second())));

                propagator.attachClause(cr);
                trail.uncheckedEnqueue(cr->first(), cr);
                claBumpActivity(*cr);
            }
            claDecayActivity();
            branch.varDecayActivity();
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
                        propagator.sortWatchers();
                        Statistics::getInstance().runtimeStop("Sort Watches");
                    }
                }

                branch.rebuildOrderHeap(trail);
                
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
            while (trail.decisionLevel() < assumptions.size()) {
                // Perform user provided assumption:
                Lit p = assumptions[trail.decisionLevel()];
                if (trail.value(p) == l_True) {
                    // Dummy decision level:
                    trail.newDecisionLevel();
                } else if (trail.value(p) == l_False) {
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
            trail.newDecisionLevel();
            trail.uncheckedEnqueue(next);
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
        printf("c |   * Trail  Queue : %6d      |   * Inc       : %6d         |                          |\n", trailQueue.maxSize(), incReduceDB);
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
            analyzeFinal(trail[trail.size()-1], finalConflict);
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
        model.insert(model.end(), trail.assigns.begin(), trail.assigns.end());

        sonification.stop(0);
    }
    else {
        sonification.stop(-1);
    }
    
    cancelUntil(0);
    
    Statistics::getInstance().runtimeStop("Solver");
    return status;
}

}

#endif
