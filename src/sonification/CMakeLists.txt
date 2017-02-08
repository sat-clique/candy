add_library(sonification
    Sonification.cc
    Sonification.h
    SolverSonification.cc
    SolverSonification.h)

if(CANDY_ENABLE_SONIFICATION)
  # TODO: use find_package, or warn the user that oscpack needs to be installed
  target_link_libraries(sonification oscpack)
endif()
