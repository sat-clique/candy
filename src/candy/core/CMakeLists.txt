add_library(core
    Solver.cc
    Solver.h
    SolverStatistics.cc
    SolverStatistics.h
    Utilities.cc
    Clause.cc
    Clause.h
    ClauseAllocator.cc
    ClauseAllocator.h
    Certificate.cc
    Certificate.h
    BoundedQueue.h
    CNFProblem.h
    Constants.h
    SolverTypes.h
    Utilities.h
)

#set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)
