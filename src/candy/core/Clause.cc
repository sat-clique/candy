/*
 * Clause.cc
 *
 *  Created on: Feb 15, 2017
 *      Author: markus
 */

#include <candy/core/Clause.h>
#include <candy/core/ClauseAllocator.h>

namespace Candy {

ClauseAllocator* Clause::allocator = new ClauseAllocator(100, 500);

Clause::Clause(const std::vector<Lit>& ps, bool learnt) {
    std::copy(ps.begin(), ps.end(), literals);
    length = ps.size();
    header = 0;
    setLearnt(learnt);
    setFrozen(false);

    if (learnt) {
        data.act = 0;
    } else {
        calcAbstraction();
    }
}

Clause::Clause(std::initializer_list<Lit> list) {
    std::copy(list.begin(), list.end(), literals);
    length = list.size();
    header = 0;
    setFrozen(false);
    calcAbstraction();
}

Clause::~Clause() { }

void* Clause::operator new (std::size_t size, uint16_t length) {
    return allocate(length);
}

void Clause::operator delete (void* p) {
    deallocate((Clause*)p);
}

void* Clause::allocate(uint16_t length) {
    return allocator->allocate(length);
}

void Clause::deallocate(Clause* clause) {
    allocator->deallocate(clause);
}

Lit& Clause::operator [](int i) {
    return literals[i];
}

Lit Clause::operator [](int i) const {
    return literals[i];
}

Clause::const_iterator Clause::begin() const {
    return literals;
}

Clause::const_iterator Clause::end() const {
    return literals + length;
}

Clause::iterator Clause::begin() {
    return literals;
}

Clause::iterator Clause::end() {
    return literals + length;
}

uint16_t Clause::size() const {
    return length;
}

bool Clause::contains(const Lit lit) const {
    return std::find(begin(), end(), lit) != end();
}

bool Clause::contains(const Var v) const {
    return std::find_if(begin(), end(), [v](Lit lit) { return var(lit) == v; }) != end();
}

bool Clause::isLearnt() const {
    return (bool)(header & LEARNT_MASK);
}

void Clause::setLearnt(bool learnt) {
    if (learnt) {
        header |= LEARNT_MASK;
    } else {
        header &= ~LEARNT_MASK;
    }
}

bool Clause::isDeleted() const {
    return (bool)(header & DELETED_MASK);
}

void Clause::setDeleted() {
    header |= DELETED_MASK;
}

/*
 * Frozen flag is now inverted so complete header can be used for sorting
 */
bool Clause::isFrozen() const {
    return !(bool)(header & UNFROZEN_MASK);
}

void Clause::setFrozen(bool flag) {
    if (!flag) {
        header |= UNFROZEN_MASK;
    } else {
        header &= ~UNFROZEN_MASK;
    }
}

const Lit Clause::back() const {
    return *(this->end()-1);
}

float& Clause::activity() {
    return data.act;
}

uint32_t Clause::abstraction() const {
    return data.abs;
}

void Clause::setLBD(uint16_t i) {
    uint16_t lbd_max = LBD_MASK;
    uint16_t flags = header & ~LBD_MASK;
    header = std::min(i, lbd_max);
    header |= flags;
}

uint16_t Clause::getLBD() const {
    return header & LBD_MASK;
}

uint16_t Clause::getHeader() const {
    return header;
}

void Clause::calcAbstraction() {
    uint32_t abstraction = 0;
    for (Lit lit : *this) {
        abstraction |= 1 << (var(lit) & 31);
    }
    data.abs = abstraction;
}

} /* namespace Candy */
