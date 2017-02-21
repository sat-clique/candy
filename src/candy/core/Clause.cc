/*
 * Clause.cc
 *
 *  Created on: Feb 15, 2017
 *      Author: markus
 */

#include <candy/core/Clause.h>
#include <candy/core/ClauseAllocator.h>

namespace Candy {

ClauseAllocator* Clause::allocator = new ClauseAllocator(50, 10);

Clause::Clause(const std::vector<Lit>& ps, bool learnt) {
    mark = 0;
    this->learnt = learnt;
    canbedel = 1;
    //seen = 0;
    //_unused = 0;
    setLBD(0);

    std::copy(ps.begin(), ps.end(), literals);
    length = ps.size();

    if (learnt) {
        data.act = 0;
    } else {
        calcAbstraction();
    }
}

Clause::Clause(std::initializer_list<Lit> list) {
    mark = 0;
    learnt = false;
    canbedel = 1;
    //seen = 0;
    //_unused = 0;
    setLBD(0);

    std::copy(list.begin(), list.end(), literals);
    length = list.size();

    if (learnt) {
        data.act = 0;
    } else {
        calcAbstraction();
    }
}

Clause::~Clause() { }

void* Clause::operator new (std::size_t size) throw() {
    assert(0 && "use new with number of literals like this: new (vector.size()) Clause(vector)");
    return nullptr;
}

void* Clause::operator new (std::size_t size, uint32_t length) {
    return allocate(length);
}

void Clause::operator delete (void* p) {
    deallocate((Clause*)p);
}

void* Clause::allocate(uint32_t length) {
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

uint32_t Clause::size() const {
    return length;
}

bool Clause::contains(Lit lit) {
    return std::find(begin(), end(), lit) != end();
}

bool Clause::isLearnt() const {
    return learnt;
}

uint32_t Clause::getMark() const {
    return mark;
}

void Clause::setMark(uint32_t m) {
    mark = m;
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

/**
 *  subsumes : (other : const Clause&)  ->  Lit
 *
 *  Description:
 *       Checks if clause subsumes 'other', and at the same time, if it can be used to simplify 'other'
 *       by subsumption resolution.
 *
 *    Result:
 *       lit_Error  - No subsumption or simplification
 *       lit_Undef  - Clause subsumes 'other'
 *       p          - The literal p can be deleted from 'other'
 */
Lit Clause::subsumes(const Clause& other) const {
    assert(!learnt);
    assert(!other.learnt);

    if (other.size() < size() || (data.abs & ~other.data.abs) != 0) {
        return Glucose::lit_Error;
    }

    Lit ret = Glucose::lit_Undef;

    for (Lit c : *this) {
        // search for c or ~c
        for (Lit d : other) {
            if (c == d) {
                goto ok;
            }
            else if (ret == Glucose::lit_Undef && c == ~d) {
                ret = c;
                goto ok;
            }
        }
        // did not find it
        return Glucose::lit_Error;
        ok: ;
    }

    return ret;
}

void Clause::strengthen(Lit p) {
    if (std::remove(begin(), end(), p) != end()) {
        --length;
    }
    calcAbstraction();
}

void Clause::setLBD(int i) {
    lbd = std::min(i, UINT16_MAX);
}

unsigned int Clause::getLBD() const {
    return lbd;
}

void Clause::setCanBeDel(bool b) {
    canbedel = b;
}
bool Clause::canBeDel() {
    return canbedel;
}

void Clause::calcAbstraction() {
    uint32_t abstraction = 0;
    for (Lit lit : *this) {
        abstraction |= 1 << (var(lit) & 31);
    }
    data.abs = abstraction;
}

} /* namespace Candy */
