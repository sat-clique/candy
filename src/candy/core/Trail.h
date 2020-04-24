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
#include "candy/mtl/Stamp.h"

namespace Candy {

struct VarData {
    Clause* reason;
    unsigned int level;
    VarData() :
        reason(nullptr), level(0) {}
    VarData(Clause* _reason, unsigned int _level) :
        reason(_reason), level(_level) {}
};

class Trail {
public:
    Trail() : 
        trail_size(0), qhead(0), trail(), 
        assigns(), vardata(), trail_lim(), stamp(), 
        decision(), assumptions(), variables(0), 
        nDecisions(0), nPropagations(0)
    { }

    Trail(unsigned int nVars) : Trail() {
        init(nVars);
    }

    unsigned int trail_size; // Current number of assignments (used to optimize propagate, through getting rid of capacity checking)
    unsigned int qhead; // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    std::vector<Lit> trail; // Assignment stack; stores all assigments made in the order they were made.
    std::vector<lbool> assigns; // The current assignments.
    std::vector<VarData> vardata; // Stores reason and level for each variable.
    std::vector<unsigned int> trail_lim; // Separator indices for different decision levels in 'trail'.
    Stamp<uint32_t> stamp;

    std::vector<char> decision;
	std::vector<Lit> assumptions; // Current set of assumptions provided to solve by the user.
    unsigned int variables;

    size_t nDecisions;
    size_t nPropagations;

    inline unsigned int nVars() {
        return variables;
    }

    inline void init(unsigned int nVars) {
        if (nVars > variables) {
            variables = nVars;
            assigns.resize(nVars, l_Undef);
            vardata.resize(nVars);
            trail.resize(nVars);
            stamp.grow(nVars);
            decision.resize(nVars, true);
        }
    }

    inline void clear() {
        variables = 0;
        assigns.clear();
        vardata.clear();
        qhead = 0;
        trail_size = 0;
        trail_lim.clear(); 
        decision.clear();
        assumptions.clear();
    }

    inline void reset() {
        std::fill(assigns.begin(), assigns.end(), l_Undef);
        std::fill(vardata.begin(), vardata.end(), VarData { nullptr, 0 });
        qhead = 0;
        trail_size = 0;
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
        this->assumptions.clear();
        for (Lit lit : assumptions) {
            if (lit.var() > (Var)variables) {
                init(variables);
            }
            this->assumptions.push_back(lit);
        }
    }

    bool hasAssumptionsNotSet() {
        return decisionLevel() < assumptions.size();
    }

    Lit nextAssumption() {
        return assumptions[decisionLevel()];
    }

    inline const Lit operator [](unsigned int i) const {
        assert(i < trail_size);
        return trail.at(i);//[i];
    }

    typedef std::vector<Lit>::const_iterator const_iterator;
    typedef std::vector<Lit>::const_reverse_iterator const_reverse_iterator;

    inline const_iterator begin() const {
        return trail.begin();
    }

    inline const_iterator end() const {
        return trail.begin() + trail_size;
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

    inline unsigned int size(unsigned int level) const {
        if (level == 0) {
            if (trail_lim.size() == 0) {
                return trail_size;
            }
            else {
                return trail_lim[0];
            }
        }
        else if (trail_lim.size() > level) {
            return trail_lim[level] - trail_lim[level-1];
        }
        else {
            return 0;
        }
    }

    // The current value of a variable.
    inline lbool value(Var x) const {
        return assigns[x];
    }

    // The current value of a literal.
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

    // Main internal methods:
    inline Clause* reason(Var x) const {
        return vardata[x].reason;
    }

    inline unsigned int level(Var x) const {
        return vardata[x].level;
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
        // std::cout << "Branching on " << p << std::endl;
        assert(value(p) == l_Undef);
        set_value(p);
        vardata[p.var()] = VarData(nullptr, decisionLevel());
        nDecisions++;
    }

    inline bool propagate(Lit p, Clause* from) {
        // std::cout << "Propgating " << p << " due to " << *from << std::endl;
        if (this->falsifies(p)) {
            return false;
        }
        else {
            set_value(p);
            vardata[p.var()] = VarData(from, decisionLevel());
            nPropagations++;
            return true;
        }
    }

    inline bool fact(Lit p) {
        // std::cout << "Setting fact " << p << std::endl;
        assert(decisionLevel() == 0);
        if (this->falsifies(p)) {
            return false;
        }
        vardata[p.var()] = VarData(nullptr, 0);
        if (!this->satisfies(p))  {
            set_value(p);
            nPropagations++;
        }
        return true;
    }

    inline void backtrack(unsigned int level) {
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
    inline uint_fast16_t computeLBD(Iterator it, Iterator end) {
        // TODO: exclude selectors from lbd computation
        uint_fast16_t nblevels = 0;
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
                std::cout << trail[i] << " ";
            }
        }
        std::cout << std::endl;
    }

};
}

#endif /* SRC_CANDY_CORE_TRAIL_H_ */

