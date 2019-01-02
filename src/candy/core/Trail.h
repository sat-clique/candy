/*
 * Trail.h
 *
 *  Created on: Jun 28, 2017
 *      Author: markus
 */

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
        trail_size(0), qhead(0), trail(), assigns(), vardata(), trail_lim(), stamp() 
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

    inline Lit& operator [](unsigned int i) {
        assert(i < trail_size);
        return trail[i];
    }

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

    typedef std::vector<Lit>::iterator iterator;
    typedef std::vector<Lit>::const_iterator const_iterator;
    typedef std::vector<Lit>::reverse_iterator reverse_iterator;
    typedef std::vector<Lit>::const_reverse_iterator const_reverse_iterator;

    inline const_iterator begin() const {
        return trail.begin();
    }

    inline const_iterator end() const {
        return trail.begin() + trail_size;
    }

    inline iterator begin() {
        return trail.begin();
    }

    inline iterator end() {
        return trail.begin() + trail_size;
    }

    inline const_reverse_iterator rbegin() const {
        return trail.rbegin() + (trail.size() - trail_size);
    }

    inline const_reverse_iterator rend() const {
        return trail.rend();
    }

    inline reverse_iterator rbegin() {
        return trail.rbegin() + (trail.size() - trail_size);
    }

    inline reverse_iterator rend() {
        return trail.rend();
    }

    inline const_iterator begin(unsigned int level) const {
        return trail.begin() + trail_lim[level];
    }

    inline iterator begin(unsigned int level) {
        return trail.begin() + trail_lim[level]; 
    }

    inline unsigned int size() const {
        return trail_size;
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

    // Returns TRUE if a clause is satisfied in the current state.
    inline bool satisfies(const Clause& c) const {
        return std::any_of(c.begin(), c.end(), [this] (Lit lit) { return value(lit) == l_True; });
    }

    inline bool isAssigned(Var v) const {
        return assigns[v] != l_Undef;
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

    // Returns TRUE if a clause is a reason for some implication in the current state.
    inline bool locked(const Clause* cr) const {
        const Clause& c = *cr;
        if (c.size() > 2) return value(c[0]) == l_True && reason(var(c[0])) == cr;
        return (value(c[0]) == l_True && reason(var(c[0])) == cr) || (value(c[1]) == l_True && reason(var(c[1])) == cr);
    }

    inline bool newFact(Lit p) {
        assert(decisionLevel() == 0);

        if (value(p) == l_False) {
            return false;
        }
        else if (value(p) == l_True) {
            vardata[var(p)] = VarData(nullptr, 0);
        }
        else {
            uncheckedEnqueue(p);
        }
        return true;
    }

    inline void uncheckedEnqueue(Lit p, Clause* from = nullptr) {
        assert(value(p) == l_Undef);
        assigns[var(p)] = lbool(!sign(p));
        vardata[var(p)] = VarData(from, decisionLevel());
        trail[trail_size++] = p;
    }

    inline void cancelUntil(unsigned int level) {
        if (decisionLevel() > level) {
            for (auto it = begin(level); it != end(); it++) {
                assigns[var(*it)] = l_Undef;
            }
            qhead = trail_lim[level];
            trail_size = trail_lim[level];
            trail_lim.erase(trail_lim.begin() + level, trail_lim.end());
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

