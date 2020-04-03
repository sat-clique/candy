add_library(sonification OBJECT
    Sonification.h
    ControllerInterface.h
)

set_target_properties(sonification PROPERTIES LINKER_LANGUAGE CXX)
