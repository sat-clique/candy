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
        trail_size(0), qhead(0), trail(), assigns(), vardata(), trail_lim(), stamp(), 
        nDecisions(0), nPropagations(0)
    { }

    Trail(unsigned int size) : Trail() { 
        grow(size); 
    }

    unsigned int trail_size; // Current number of assignments (used to optimize propagate, through getting rid of capacity checking)
    unsigned int qhead; // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    std::vector<Lit> trail; // Assignment stack; stores all assigments made in the order they were made.
    std::vector<lbool> assigns; // The current assignments.
    std::vector<VarData> vardata; // Stores reason and level for each variable.
    std::vector<unsigned int> trail_lim; // Separator indices for different decision levels in 'trail'.
    Stamp<uint32_t> stamp;

    size_t nDecisions;
    size_t nPropagations;

    inline const Lit operator [](unsigned int i) const {
        assert(i < trail_size);
        return trail[i];
    }

    void print() {
        unsigned int level = 0;
        std::cout << "Trail: ";
        for (unsigned int i = 0; i < size(); i++) {
            if (i == trail_lim[level]) {
                std::cout << "'" << trail[i] << "' ";
                level++;
            } 
            else {
                std::cout << trail[i] << " ";
            }
        }
        std::cout << std::endl;
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

    inline void grow() {
        assigns.push_back(l_Undef);
        vardata.emplace_back();
        trail.push_back(lit_Undef);
        stamp.grow();
    }

    inline void grow(size_t size) {
        if (size > trail.size()) {
            assigns.resize(size, l_Undef);
            vardata.resize(size);
            trail.resize(size);
            stamp.grow(size);
        }
    }

    // The current value of a variable.
    inline lbool value(Var x) const {
        return assigns[x];
    }

    // The current value of a literal.
    inline lbool value(Lit p) const {
        return assigns[var(p)] ^ sign(p);
    }

    inline bool satisfies(Lit lit) const {
        return value(lit) == l_True;
    }

    inline bool satisfies(const Clause& c) const {
        return std::any_of(c.begin(), c.end(), [this] (Lit lit) { return value(lit) == l_True; });
    }

    inline bool falsifies(Lit lit) const {
        return value(lit) == l_False;
    }

    inline bool falsifies(const Clause& c) const {
        return std::all_of(c.begin(), c.end(), [this] (Lit lit) { return value(lit) == l_False; });
    }

    inline bool defines(Lit lit) const {
        return value(lit) != l_Undef;
    }

    inline bool defines(const Clause& c) const {
        return std::all_of(c.begin(), c.end(), [this] (Lit lit) { return value(lit) != l_Undef; });
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

    inline void decide(Lit p) {
        assert(value(p) == l_Undef);
        assigns[var(p)] = lbool(!sign(p));
        vardata[var(p)] = VarData(nullptr, decisionLevel());
        trail[trail_size++] = p;
        nDecisions++;
    }

    inline bool propagate(Lit p, Clause* from) {
        if (this->falsifies(p)) {
            return false;
        }
        else {
            assigns[var(p)] = lbool(!sign(p));
            vardata[var(p)] = VarData(from, decisionLevel());
            trail[trail_size++] = p;
            nPropagations++;
            return true;
        }
    }

    inline bool fact(Lit p) {
        assert(decisionLevel() == 0);
        vardata[var(p)] = VarData(nullptr, 0);
        if (!this->defines(p)) {
            assigns[var(p)] = lbool(!sign(p));
            trail[trail_size++] = p;
            nPropagations++;
            return true;
        }
        else {
            return this->satisfies(p);
        }
    }

    inline void backtrack(unsigned int level) {
        if (decisionLevel() > level) {
            for (auto it = begin(level); it != end(); it++) {
                assigns[var(*it)] = l_Undef;
            }
            qhead = trail_lim[level];
            trail_size = trail_lim[level];
            trail_lim.erase(trail_lim.begin() + level, trail_lim.end());
        }
    }

    inline void reset() {
        for (Lit lit : *this) {
            assigns[var(lit)] = l_Undef;
        }
        qhead = 0;
        trail_size = 0;
        trail_lim.clear(); 
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
            int l = level(var(lit));
            if (!stamp[l]) {
                stamp.set(l);
                nblevels++;
            }
        }

        return nblevels;
    }

};
}

#endif /* SRC_CANDY_CORE_TRAIL_H_ */

