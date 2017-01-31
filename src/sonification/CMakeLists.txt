add_library(sonification
    Sonification.cc
    Sonification.h
    SolverSonification.cc
    SolverSonification.h)

include_directories("/usr/local/include/oscpack/")

target_link_libraries(sonification oscpack)
