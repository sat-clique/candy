add_library(core OBJECT
    Solver.h
    clauses/Clause.h
    clauses/ClauseAllocator.h
    clauses/ClauseAllocatorMemory.h
    clauses/ClauseDatabase.h
    clauses/Certificate.h
    CNFProblem.cc
    CNFProblem.h
    DRATChecker.cc
    DRATChecker.h
    SolverTypes.h
    Trail.h
)

#set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)
