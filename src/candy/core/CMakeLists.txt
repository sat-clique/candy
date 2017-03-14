add_library(core
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
    CNFProblem.h
    SolverTypes.h
)

#set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)
