add_executable(randomsimulation_tests Main.cc
    PartitionTests.cc
    PropagationTests.cc
    ClauseOrderTests.cc
    TestGateStructure.h
    TestGateStructure.cc
    SimulationVectorTests.cc
    TestUtils.cc
    TestUtils.h
    TestGateStructureTests.cc
    RandomSimulatorTests.cc)

target_include_directories(randomsimulation_tests PUBLIC ${CMAKE_PROJECT_ROOT}/lib/googletest/googletest/include
                                                         ${CMAKE_PROJECT_ROOT}/lib/googletest/googlemock/include)
target_link_libraries(randomsimulation_tests ${CANDY_LIBS} gtest)
candy_executable_customizations(randomsimulation_tests)
