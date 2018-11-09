#ifndef TEST_CLAUSE_FACTORY_H_
#define TEST_CLAUSE_FACTORY_H_

#include "candy/core/Clause.h"
#include "candy/core/ClauseAllocator.h"
#include "candy/core/SolverTypes.h"

namespace Candy {

class TestClauseFactory {
    ClauseAllocator allocator;

public:
    Clause* getClause(std::initializer_list<Lit> list) {
        return new (allocator.allocate(list.size())) Clause(list);
    }

    Clause* getClauseWithLBD(std::initializer_list<Lit> list, int lbd) {
        Clause* clause = new (allocator.allocate(list.size())) Clause(list);
        clause->setLBD(lbd);
        return clause;
    }

    Clause* getClauseFrozen(std::initializer_list<Lit> list) {
        Clause* clause = new (allocator.allocate(list.size())) Clause(list);
        clause->setFrozen(true);
        return clause;
    }
    
    Clause* getClauseLearnt(std::initializer_list<Lit> list) {
        Clause* clause = new (allocator.allocate(list.size())) Clause(list);
        clause->setLearnt(true);
        return clause;
    }
    
    Clause* getClauseDeleted(std::initializer_list<Lit> list) {
        Clause* clause = new (allocator.allocate(list.size())) Clause(list);
        clause->setDeleted();
        return clause;
    }
    
    Clause* getClauseFrozenWithLBD(std::initializer_list<Lit> list, int lbd) {
        Clause* clause = new (allocator.allocate(list.size())) Clause(list);
        clause->setFrozen(true);
        clause->setLBD(lbd);
        return clause;
    }
    
    Clause* getClauseLearntWithLBD(std::initializer_list<Lit> list, int lbd) {
        Clause* clause = new (allocator.allocate(list.size())) Clause(list);
        clause->setLearnt(true);
        clause->setLBD(lbd);
        return clause;
    }
    
    Clause* getClauseDeletedWithLBD(std::initializer_list<Lit> list, int lbd) {
        Clause* clause = new (allocator.allocate(list.size())) Clause(list);
        clause->setDeleted();
        clause->setLBD(lbd);
        return clause;
    }
    

};

}

#endif 