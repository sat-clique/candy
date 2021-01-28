add_library(simplification OBJECT
    OccurenceList.h
    Subsumption.h
    VariableElimination.h
)

set_target_properties(simplification PROPERTIES LINKER_LANGUAGE CXX)
