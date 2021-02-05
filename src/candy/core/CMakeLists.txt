add_library(core OBJECT
    Solver.h
    SolverTypes.h
    Trail.h
    ReduceDB.h
    Restart.h
    clauses/Clause.h
    clauses/ClauseAllocator.h
    clauses/ClauseAllocatorMemory.h
    clauses/ClauseDatabase.h
    clauses/Certificate.h
    clauses/BinaryClauses.h
    clauses/NaryClauses.h
    clauses/Equivalences.h
    systems/BranchingInterface.h
    systems/BranchingVSIDS.h
    systems/BranchingLRB.h
    systems/LearningInterface.h
    systems/Learning1UIP.h
    systems/PropagationInterface.h
    systems/Propagation2WL.h
    systems/Propagation2WLStatic.h
    systems/PropagationLB.h
    systems/Propagation2WL3Full.h
    CNFProblem.cc
    CNFProblem.h
    DRATChecker.cc
    DRATChecker.h
)

#set_target_properties(core PROPERTIES LINKER_LANGUAGE CXX)
