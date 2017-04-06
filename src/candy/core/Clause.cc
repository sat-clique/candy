/*
 * Clause.cc
 *
 *  Created on: Feb 15, 2017
 *      Author: markus
 */

#include <candy/core/Clause.h>
#include <candy/core/ClauseAllocator.h>

namespace Candy {

Clause::Clause(const std::vector<Lit>& ps, uint16_t lbd) {
    std::copy(ps.begin(), ps.end(), literals);
    length = ps.size();
    header = 0;
    setLearnt(true); // only learnts have lbd
    setLBD(lbd);
    data.activity = 0;
    assert(std::unique(begin(), end()) == end());
}

Clause::Clause(std::initializer_list<Lit> list) {
    std::copy(list.begin(), list.end(), literals);
    length = list.size();
    header = 0; // not frozen, not deleted and not learnt; lbd=0
    calcAbstraction();
}

Clause::~Clause() { }

bool Clause::contains(const Lit lit) const {
    return std::find(begin(), end(), lit) != end();
}

bool Clause::contains(const Var v) const {
    return std::find_if(begin(), end(), [v](Lit lit) { return var(lit) == v; }) != end();
}

void Clause::calcAbstraction() {
    uint32_t abstraction = 0;
    for (Lit lit : *this) {
        abstraction |= 1 << (var(lit) & 31);
    }
    data.abstraction = abstraction;
}

} /* namespace Candy */
