add_library(randomsimulation
    RandomSimulator.cc
    RandomSimulator.h
    SimulationVector.cc
    SimulationVector.h
    Randomization.cc
    Randomization.h
    Partition.cc
    Partition.h
    ClauseOrder.cc
    ClauseOrder.h)

target_compile_options(randomsimulation PRIVATE -Wextra -Weffc++)
