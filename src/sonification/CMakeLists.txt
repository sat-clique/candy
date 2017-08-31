add_library(sonification OBJECT
    Sonification.cc
    Sonification.h
    SolverSonification.cc
    SolverSonification.h)

if(CANDY_ENABLE_SONIFICATION)
  # TODO: use find_package, or warn the user that oscpack needs to be installed
  # BUG: cmake object libraries may not have dependencies; link in top-level cmakelists file
  #target_link_libraries(sonification oscpack)
endif()
