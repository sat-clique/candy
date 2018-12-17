add_library(core OBJECT
    Solver.h
    Statistics.cc
    Statistics.h
    Clause.cc
    Clause.h
    ClauseAllocator.h
    ClauseDatabase.cc
    ClauseDatabase.h
    Certificate.cc
    Certificate.h
    CNFProblem.cc
    CNFProblem.h
    SolverTypes.h
    Trail.h
    learning/ConflictAnalysis.h
    learning/ConflictAnalysisThreadSafe.h
    propagate/Propagate.h
    propagate/PropagateThreadSafe.h
    branching/BranchingDiversificationInterface.h
    branching/VSIDS.h
    branching/LRB.h
    simplification/Subsumption.h
    simplification/VariableElimination.h
)

#set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)
