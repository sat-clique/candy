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
#include <iostream>

namespace Candy {

#define BITS_LBD 13

class Clause {
    struct {
        unsigned mark :2;
        unsigned learnt :1;
        unsigned canbedel :1;
    } header;

    union {
        float act;
        uint32_t abs;
    } data;

    uint16_t lbd;
    uint16_t length;

    Lit literals[1];

private:
    void* operator new (std::size_t size) throw() { assert(0); return nullptr; };

protected:
    static ClauseAllocator* allocator;

public:
    Clause(const std::vector<Lit>& ps, bool learnt);
    Clause(std::initializer_list<Lit> list);
    virtual ~Clause();

    void* operator new (std::size_t size, uint16_t length);
    void operator delete (void* p);

    static void* allocate(uint16_t length);
    static void deallocate(Clause* clause);

    typedef Lit* iterator;
    typedef const Lit* const_iterator;

    Lit& operator [](int i);
    Lit operator [](int i) const;

    const_iterator begin() const;
    const_iterator end() const;
    iterator begin();
    iterator end();
    uint16_t size() const;

    void calcAbstraction();

    const Lit back() const;
    bool contains(Lit lit);
    bool contains(Var var);

    void swap(uint16_t pos1, uint16_t pos2);

    bool isLearnt() const;
    uint32_t getMark() const;
    void setMark(uint32_t m);

    float& activity();
    uint32_t abstraction() const;

    Lit subsumes(const Clause& other) const;
    void strengthen(Lit p);

    void setLBD(uint16_t i);
    uint16_t getLBD() const;
    void setCanBeDel(bool b);
    bool canBeDel();

    static void printAlignment() {
        Clause clause({Glucose::lit_Undef});
        uint64_t start = (uint64_t)&clause;
        uint64_t header = (uint64_t)&(clause.header);
        uint64_t data = (uint64_t)&(clause.data);
        uint64_t lbd = (uint64_t)&(clause.lbd);
        uint64_t length = (uint64_t)&(clause.length);
        uint64_t literals = (uint64_t)&(clause.literals);
        std::cout << "c Size of Clause: " << sizeof(Candy::Clause) << std::endl;
        std::cout << "c Header starts at " << header - start << std::endl;
        std::cout << "c Data-union starts at " << data - start << std::endl;
        std::cout << "c LBD starts at " << lbd - start << std::endl;
        std::cout << "c Length starts at " << length - start << std::endl;
        std::cout << "c Literals start at " << literals - start << std::endl;
    }
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSE_H_ */
