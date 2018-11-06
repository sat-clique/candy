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
#include "candy/core/Clause.h"
#include "candy/core/Stamp.h"

namespace Candy {

struct VarData {
    Clause* reason;
    uint_fast32_t level;
    VarData() :
        reason(nullptr), level(0) {}
    VarData(Clause* _reason, uint_fast32_t _level) :
        reason(_reason), level(_level) {}
};

class Trail {
private:
    std::vector<Lit> backtracked;

public:
    Trail() : 
        backtracked(), trail_size(0), qhead(0), trail(), assigns(), vardata(), trail_lim(), stamp() 
    { }

    Trail(uint32_t size) : Trail() { 
        grow(size); 
    }

    unsigned int trail_size; // Current number of assignments (used to optimize propagate, through getting rid of capacity checking)
    unsigned int qhead; // Head of queue (as index into the trail -- no more explicit propagation queue in MiniSat).
    std::vector<Lit> trail; // Assignment stack; stores all assigments made in the order they were made.
    std::vector<lbool> assigns; // The current assignments.
    std::vector<VarData> vardata; // Stores reason and level for each variable.
    std::vector<unsigned int> trail_lim; // Separator indices for different decision levels in 'trail'.
    Stamp<uint32_t> stamp;

    inline std::vector<Lit> getBacktracked() {
        return backtracked;
    }

    inline Lit& operator [](unsigned int i) {
        assert(i < trail_size);
        return trail[i];
    }

    inline const Lit operator [](unsigned int i) const {
        assert(i < trail_size);
        return trail[i];
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

    inline bool isAssigned(Var v) const {
        return assigns[v] != l_Undef;
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
    inline size_t decisionLevel() const {
        return trail_lim.size();
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
        // std::cout << "Assign " << p;
        // if (from != nullptr) std::cout << " because " << *from;
        // else std::cout << std::endl;
    }

    inline void cancelUntil(unsigned int level) {
        backtracked.clear();
        if (decisionLevel() > level) {
            backtracked.insert(backtracked.end(), begin() + trail_lim[level], end());
            // std::cout << "Backtrack " << backtracked;
            for (Lit lit : backtracked) {
                assigns[var(lit)] = l_Undef;
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

	// DYNAMIC NBLEVEL trick (see competition'09 Glucose companion paper)
    void reduceLBDs(std::vector<Clause*>& involved_clauses) {
        for (Clause* clause : involved_clauses) {
            if (clause->isLearnt()) {
                uint_fast16_t nblevels = computeLBD(clause->begin(), clause->end());
                if (nblevels + 1 < clause->getLBD()) {
                    clause->setLBD(nblevels); // improve the LBD
                    clause->setFrozen(true); // Seems to be interesting, keep it for the next round
                }
            }
        }
    }

};
}

#endif /* SRC_CANDY_CORE_TRAIL_H_ */

