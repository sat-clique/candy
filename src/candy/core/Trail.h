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

#ifndef SRC_CANDY_CORE_TRAIL_H_
#define SRC_CANDY_CORE_TRAIL_H_

#include <vector>
#include "candy/core/SolverTypes.h"
#include "candy/core/clauses/Clause.h"
#include "candy/core/CNFProblem.h"
#include "candy/mtl/Stamp.h"

namespace Candy {

#define BIT64 (1ULL << 63)

class Reason {
    union D {
        uintptr_t raw;
        Clause* clause;
        Lit binary[2];
        D() : raw(0) {}
    } data;

public:
    Reason() {
        unset();
    }

    Reason(Clause* clause) {
        set(clause);
    }

    Reason(Lit lit1, Lit lit2) {
        set(lit1, lit2);
    }

    void unset() {
        data.raw = 0;
    }

    bool exists() const {
        return data.raw != 0;
    }

    void set(Clause* clause) {
        assert(clause != nullptr);
        data.clause = clause;
        data.raw |= BIT64;
    }

    void set(Lit lit1, Lit lit2) {
        data.binary[0] = lit1;
        data.binary[1] = lit2;
    }

    inline bool is_ptr() const {
        return data.raw & BIT64;
    }

    inline Clause* get_ptr() const {
        return (Clause*)(data.raw & ~BIT64);
    }

    typedef const Lit* const_iterator;

    inline const_iterator begin() const {
        if (is_ptr()) {
            return get_ptr()->begin();
        } else {
            return data.binary;
        }
    }

    inline const_iterator end() const {
        if (is_ptr()) {
            return get_ptr()->end();
        } else {
            return data.binary + 2;
        }
    }
};

inline std::ostream& operator <<(std::ostream& stream, Reason const& reason) {
    for (Lit lit : reason) {
        stream << lit << " ";
    }
    return stream;
}

class Trail {
public:
    unsigned int nVariables;
    unsigned int trail_size; // Current number of assignments (used to optimize propagate, through getting rid of capacity checking)
    unsigned int conflict_level; // stores level of last conflict
    unsigned int qhead; // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).

    std::vector<Lit> trail; // Assignment stack; stores all assigments made in the order they were made.
    std::vector<lbool> assigns; // The current assignments.
    std::vector<unsigned int> levels; // decision-level of assignment per variable
    std::vector<Reason> reasons; // reason of assignment per variable
    std::vector<unsigned int> trail_lim; // Separator indices for different decision levels in 'trail'.
    Stamp<uint32_t> stamp;

    std::vector<char> decision;
	std::vector<Lit> assumptions; // Current set of assumptions provided to solve by the user.
    std::vector<Lit> conflicting_assumptions; // Set of conflicting assumptions (analyze_final)

    size_t nDecisions;
    size_t nPropagations;

    Trail(CNFProblem& problem) : 
        nVariables(problem.nVars()), trail_size(0), conflict_level(0), qhead(0), 
        trail(), assigns(), levels(), reasons(), trail_lim(), stamp(problem.nVars()), 
        decision(), assumptions(), conflicting_assumptions(), 
        nDecisions(0), nPropagations(0)
    { 
        trail.resize(problem.nVars());
        assigns.resize(problem.nVars(), l_Undef);
        levels.resize(problem.nVars());
        reasons.resize(problem.nVars());
        decision.resize(problem.nVars(), true);
    }

    inline unsigned int nVars() {
        return nVariables;
    }

    inline void reset() {
        trail_size = 0;
        conflict_level = 0;
        qhead = 0;
        std::fill(assigns.begin(), assigns.end(), l_Undef);
        std::fill(levels.begin(), levels.end(), 0);
        std::fill(reasons.begin(), reasons.end(), Reason());
        trail_lim.clear(); 
    }

    // Declare if a variable should be eligible for selection in the decision heuristic.
    void setDecisionVar(Var v, bool b) {
        decision[v] = b; // make sure to reset decision heuristics data-structures
    }

    bool isDecisionVar(Var v) {
        return decision[v]; 
    }

    void setAssumptions(const std::vector<Lit>& assumptions) {
        for (Lit lit : this->assumptions) {
            setDecisionVar(lit.var(), true);
        }
        this->assumptions.clear();
        this->conflicting_assumptions.clear();
        for (Lit lit : assumptions) {
            assert(lit.var() < (Var)nVariables);
            setDecisionVar(lit.var(), false);
            this->assumptions.push_back(lit);
        }
    }

    Lit nextAssumption() {
        while (decisionLevel() < assumptions.size()) {
            Lit p = assumptions[decisionLevel()];
            if (value(p) == l_True) {
                newDecisionLevel(); // Dummy decision level
            } 
            else if (value(p) == l_False) {
                analyzeFinal(~p);
                return lit_Error;
            } 
            else {
                return p;
            }
        }
        return lit_Undef;
    }

	/**************************************************************************************************
	 *
	 *  analyzeFinal : (p : Lit)  ->  std::vector<Lit>
	 *
	 *  Specialized analysis procedure to express the final conflict in terms of assumptions.
	 *  Calculates and returns the set of assumptions that led to the assignment of 'p'.
	 * 
	 |*************************************************************************************************/
	void analyzeFinal(Lit p) { 
		conflicting_assumptions.clear();
	    conflicting_assumptions.push_back(p);

        if (decisionLevel() == 0) return;

        stamp.clear();
        stamp.set(p.var());

        for (int i = trail_size - 1; i >= (int)trail_lim[0]; i--) {
            Var x = trail[i].var();
            if (stamp[x]) {
                if (reason(x).exists()) {
                    for (Lit lit : reason(x)) {
                        stamp.set(lit.var());
                    }
                } else {
                    assert(level(x) > 0);
                    conflicting_assumptions.push_back(~trail[i]);
                }
            }
        }
	}

    inline const Lit operator [](unsigned int i) const {
        assert(i < trail_size);
        return trail[i];
    }

    typedef std::vector<Lit>::const_iterator const_iterator;
    typedef std::vector<Lit>::const_reverse_iterator const_reverse_iterator;

    inline const_iterator begin() const {
        return trail.begin();
    }

    inline const_iterator end() const {
        return trail.begin() + trail_size;
    }

    inline const_reverse_iterator conflict_rbegin() const {
        return trail.rbegin() + (trail.size() - conflict_level);
    }

    inline const_reverse_iterator rbegin() const {
        return trail.rbegin() + (trail.size() - trail_size);
    }

    inline const_reverse_iterator rend() const {
        return trail.rend();
    }

    inline const_iterator begin(unsigned int level) const {
        return trail.begin() + trail_lim[level];
    }

    inline unsigned int size() const {
        return trail_size;
    }

    inline lbool value(Var x) const {
        return assigns[x];
    }

    inline lbool value(Lit p) const {
        return assigns[p.var()] ^ p.sign();
    }

    inline bool satisfies(Lit lit) const {
        return value(lit) == l_True;
    }

    template<typename Iterator>
    inline bool satisfies(Iterator begin, Iterator end) const {
        return std::any_of(begin, end, [this] (Lit lit) { return value(lit) == l_True; });
    }

    inline bool falsifies(Lit lit) const {
        return value(lit) == l_False;
    }

    template<typename Iterator>
    inline bool falsifies(Iterator begin, Iterator end) const {
        return std::all_of(begin, end, [this] (Lit lit) { return value(lit) == l_False; });
    }

    inline Reason reason(Var x) const {
        return reasons[x];
    }

    inline unsigned int level(Var x) const {
        return levels[x];
    }

    // Gives the current decisionlevel.
    inline unsigned int decisionLevel() const {
        return trail_lim.size();
    }

    // Begins a new decision level
    inline void newDecisionLevel() {
        trail_lim.push_back(trail_size);
    }

    inline void set_value(Lit p) {
        assigns[p.var()] = lbool(!p.sign());
        trail[trail_size++] = p;
    }

    inline void decide(Lit p) {
        assert(value(p) == l_Undef);
        set_value(p);
        reasons[p.var()].unset();
        levels[p.var()] = decisionLevel();
        nDecisions++;
    }

    inline bool propagate(Lit p, Reason reason) {
        assert(value(p) != l_True);
        lbool val = value(p);
        if (val != l_False) {
            set_value(p);
            reasons[p.var()] = reason;
            levels[p.var()] = decisionLevel();
            nPropagations++;
            return true;
        }
        return false;
    }

    inline bool fact(Lit p) {
        assert(decisionLevel() == 0);        
        lbool val = value(p);
        if (val != l_False) {
            if (val == l_Undef) {
                set_value(p);
            }
            reasons[p.var()].unset();
            levels[p.var()] = 0;
            return true;
        }
        return false;
    }

    inline void backtrack(unsigned int level) {
        conflict_level = trail_size;
        if (decisionLevel() > level) {
            for (auto it = begin(level); it != end(); it++) {
                assigns[it->var()] = l_Undef; 
            }
            qhead = level == 0 ? 0 : trail_lim[level];
            trail_size = trail_lim[level];
            trail_lim.resize(level);
        }
    }

    /**
     * Count the number of decision levels in which the given list of literals was assigned
     */
    template <typename Iterator>
    inline uint8_t computeLBD(Iterator it, Iterator end) {
        // TODO: exclude selectors from lbd computation
        uint8_t nblevels = 0;
        stamp.clear();

        for (; it != end; it++) {
        	Lit lit = *it;
            int l = level(lit.var());
            if (!stamp[l]) {
                stamp.set(l);
                nblevels++;
            }
        }

        return nblevels;
    }

    void print() {
        unsigned int level = 0;
        std::cout << "cT Trail (size = " << trail_size << ", levels = " << trail_lim.size() << "): " << std::endl << "cT Level 0: ";
        for (unsigned int i = 0; i < trail_size; ++i) {
            if (level < trail_lim.size() && i == trail_lim[level]) {
                level++;
                std::cout << std::endl << "cT Level " << level << ": '" << trail[i] << "' ";
            } 
            else {
                std::cout << trail[i] << " (" << reason(trail[i].var()) << ") ";
            }
        }
        std::cout << std::endl;
    }

};
}

#endif

