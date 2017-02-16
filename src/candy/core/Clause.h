/*
 * Clause.h
 *
 *  Created on: Feb 15, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CLAUSE_H_
#define SRC_CANDY_CORE_CLAUSE_H_

#include "candy/core/SolverTypes.h"

namespace Candy {

#define BITS_LBD 13
//#define BITS_SIZEWITHOUTSEL 19
//#define BITS_REALSIZE 21

class Clause {

    struct {
        unsigned mark :2;
        unsigned learnt :1;
        unsigned canbedel :1;
        unsigned seen :1;
        unsigned lbd :BITS_LBD;
        //unsigned _unused :14; // Unused bits of 32
    } header;

    union {
        float act;
        uint32_t abs;
    } data;

    std::vector<Lit> literals;

public:
    Clause(const std::vector<Lit>& ps, bool learnt);
    Clause(std::initializer_list<Lit> list);
    virtual ~Clause();

    void calcAbstraction();

    bool contains(Lit lit);

    int size() const;
    void shrink(int i);
    void pop_back();
    bool learnt() const;
    bool has_extra() const;
    uint32_t mark() const;
    void mark(uint32_t m);
    const Lit& last() const;

    // NOTE: somewhat unsafe to change the clause in-place! Must manually call 'calcAbstraction' afterwards for
    //       subsumption operations to behave correctly.
    Lit& operator [](int i);
    Lit operator [](int i) const;

    float& activity();
    uint32_t abstraction() const;

    Lit subsumes(const Clause& other) const;
    void strengthen(Lit p);
    void setLBD(int i);
    unsigned int lbd() const;
    void setCanBeDel(bool b);
    bool canBeDel();
    void setSeen(bool b);
    bool getSeen();
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSE_H_ */
