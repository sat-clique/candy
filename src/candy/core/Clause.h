/*
 * Clause.h
 *
 *  Created on: Feb 15, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CLAUSE_H_
#define SRC_CANDY_CORE_CLAUSE_H_

#include "candy/core/SolverTypes.h"
#include "candy/core/ClauseAllocator.h"

namespace Candy {

class Clause {
    unsigned mark :2;
    unsigned learnt :1;
    unsigned canbedel :1;
    //unsigned seen :1;
    //unsigned _unused :11; // Unused bits of 16
    uint16_t lbd;

    union {
        float act;
        uint32_t abs;
    } data;

    uint32_t length;
    Lit literals[1];

protected:
    static ClauseAllocator* allocator;

public:
    Clause(const std::vector<Lit>& ps, bool learnt);
    Clause(std::initializer_list<Lit> list);
    virtual ~Clause();

    void* operator new (std::size_t size) throw();
    void* operator new (std::size_t size, uint32_t length);
    void operator delete (void* p);

    static void* allocate(uint32_t length);
    static void deallocate(Clause* clause);

    typedef Lit* iterator;
    typedef const Lit* const_iterator;

    Lit& operator [](int i);
    Lit operator [](int i) const;

    const_iterator begin() const;
    const_iterator end() const;
    iterator begin();
    iterator end();
    uint32_t size() const;

    void calcAbstraction();

    const Lit back() const;
    bool contains(Lit lit);
    bool contains(Var var);

    void swap(uint32_t pos1, uint32_t pos2);

    bool isLearnt() const;
    uint32_t getMark() const;
    void setMark(uint32_t m);

    float& activity();
    uint32_t abstraction() const;

    Lit subsumes(const Clause& other) const;
    void strengthen(Lit p);

    void setLBD(int i);
    unsigned int getLBD() const;
    void setCanBeDel(bool b);
    bool canBeDel();
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSE_H_ */
