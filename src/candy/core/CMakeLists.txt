add_library(core OBJECT
    Solver.cc
    Solver.h
    Statistics.cc
    Statistics.h
    Clause.cc
    Clause.h
    ClauseAllocator.cc
    ClauseAllocator.h
    Certificate.cc
    Certificate.h
    SolverTypes.h
    Trail.h
    CNFProblem.cc
    CNFProblem.h
)

#set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)
