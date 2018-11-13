add_library(core OBJECT
    Solver.h
    Statistics.cc
    Statistics.h
    Clause.cc
    Clause.h
    ClauseAllocator.cc
    ClauseAllocator.h
    ClauseDatabase.cc
    ClauseDatabase.h
    Certificate.cc
    Certificate.h
    CNFProblem.cc
    CNFProblem.h
    SolverTypes.h
    Trail.h
    ConflictAnalysis.h
    ConflictAnalysisThreadSafe.h
    Propagate.h
    PropagateThreadSafe.h
    branching/BranchingInterface.h
    branching/VSIDS.h
    branching/LRB.h
    Stamp.h
)

#set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)
