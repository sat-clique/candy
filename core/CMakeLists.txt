add_library(core
    Solver.cc
    Utilities.cc
    BoundedQueue.h
    CNFProblem.h
    Constants.h
    Solver.h
    SolverTypes.h
    Utilities.h
)

#set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)
