/*
 * Clause.cc
 *
 *  Created on: Feb 15, 2017
 *      Author: markus
 */

#include <core/Clause.h>

namespace Candy {

Clause::Clause(const std::vector<Lit>& ps, bool learnt) {
    header.mark = 0;
    header.learnt = learnt;
    header.lbd = 0;
    header.canbedel = 1;
    header.seen = 0;

    literals.insert(literals.end(), ps.begin(), ps.end());

    if (header.learnt) {
        data.act = 0;
    } else {
        calcAbstraction();
    }
}

Clause::Clause(std::initializer_list<Lit> list) {
    header.mark = 0;
    header.learnt = false;
    header.lbd = 0;
    header.canbedel = 1;
    header.seen = 0;

    literals.insert(literals.end(), list.begin(), list.end());

    if (header.learnt) {
        data.act = 0;
    } else {
        calcAbstraction();
    }
}

Clause::~Clause() { }

void Clause::calcAbstraction() {
    uint32_t abstraction = 0;
    for (Lit lit : literals) {
        abstraction |= 1 << (var(lit) & 31);
    }
    data.abs = abstraction;
}

bool Clause::contains(Lit lit) {
    return std::find(literals.begin(), literals.end(), lit) != literals.end();
}

int Clause::size() const {
    return literals.size();
}

void Clause::shrink(int i) {
    literals.resize(literals.size() - i);
}

void Clause::pop_back() {
    shrink(1);
}

bool Clause::learnt() const {
    return header.learnt;
}

uint32_t Clause::mark() const {
    return header.mark;
}

void Clause::mark(uint32_t m) {
    header.mark = m;
}

const Lit& Clause::last() const {
    return literals.back();
}

// NOTE: somewhat unsafe to change the clause in-place! Must manually call 'calcAbstraction' afterwards for
//       subsumption operations to behave correctly.
Lit& Clause::operator [](int i) {
    return literals[i];
}

Lit Clause::operator [](int i) const {
    return literals[i];
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
    assert(!header.learnt);
    assert(!other.header.learnt);

    if (other.size() < size() || (data.abs & ~other.data.abs) != 0) {
        return Glucose::lit_Error;
    }

    Lit ret = Glucose::lit_Undef;

    for (Lit c : literals) {
        // search for c or ~c
        for (Lit d : other.literals) {
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
    literals.erase(remove(literals.begin(), literals.end(), p), literals.end());
    calcAbstraction();
}

void Clause::setLBD(int i) {
    if (i < (1 << (BITS_LBD - 1))) {
        header.lbd = i;
    } else {
        header.lbd = (1 << (BITS_LBD - 1));
    }
}

unsigned int Clause::lbd() const {
    return header.lbd;
}

void Clause::setCanBeDel(bool b) {
    header.canbedel = b;
}
bool Clause::canBeDel() {
    return header.canbedel;
}

void Clause::setSeen(bool b) {
    header.seen = b;
}

bool Clause::getSeen() {
    return header.seen;
}

} /* namespace Candy */
