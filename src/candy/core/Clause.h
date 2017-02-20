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
    unsigned seen :1;
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

    void* operator new (std::size_t size) throw() {
        assert(0 && "use new with number of literals like this: new (vector.size()) Clause(vector)");
        return nullptr;
    }

    void* operator new (std::size_t size, uint32_t length) {
        return allocate(length);
    }

    void operator delete (void* p) {
        deallocate((Clause*)p);
    }

    static void* allocate(uint32_t length) {
        //void* mem = malloc(sizeof(Candy::Clause) + length * sizeof(Lit));
        return allocator->allocate(length);
    }

    static void deallocate(Clause* clause) {
        allocator->deallocate(clause);
    }

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
    void setSeen(bool b);
    bool getSeen();
};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSE_H_ */
