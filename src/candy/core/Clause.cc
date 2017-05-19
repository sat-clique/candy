/*
 * Clause.cc
 *
 *  Created on: Feb 15, 2017
 *      Author: markus
 */

#include <candy/core/Clause.h>
#include <candy/core/ClauseAllocator.h>

namespace Candy {

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

void Clause::print() const {
  for (Lit it : *this) {
    printLiteral(it);
    printf(" ");
  }
  printf("0\n");
}

void Clause::print(std::vector<lbool> values) const {
    for (Lit it : *this) {
        printLiteral(it, values);
        printf(" ");
    }
    printf("0\n");
}

} /* namespace Candy */
