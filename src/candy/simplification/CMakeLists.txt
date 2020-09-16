add_library(simplification OBJECT
    Subsumption.h
    OccurenceList.h
    VariableElimination.h
    AsymmetricVariableReduction.h
)

set_target_properties(simplification PROPERTIES LINKER_LANGUAGE CXX)
