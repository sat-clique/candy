add_library(systems OBJECT
    Restart.h
    learning/ConflictAnalysis.h
    propagate/Propagate.h
    propagate/StaticPropagate.h
    branching/BranchingDiversificationInterface.h
    branching/VSIDS.h
    branching/LRB.h
    branching/rsil/BranchingHeuristics.h
    branching/rsil/BranchingHeuristics.cc
    branching/rsil/RSARHeuristicsFilter.h
    branching/rsil/ImplicitLearningAdvice.h
)

set_target_properties(systems PROPERTIES LINKER_LANGUAGE CXX)
