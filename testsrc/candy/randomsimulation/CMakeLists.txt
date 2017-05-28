add_executable(randomsimulation_tests
    PartitionTests.cc
    PropagationTests.cc
    ClauseOrderTests.cc
    SimulationVectorTests.cc
    TestUtils.cc
    TestUtils.h
    RandomSimulatorTests.cc
    RandomizationTests.cc
    ConjectureFilterTests.cc
    GateFilterTests.cc)

candy_executable_customizations(randomsimulation_tests)

target_include_directories(randomsimulation_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)
target_include_directories(randomsimulation_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc/candy)


# add a ctest test named randomsimulation_module_tests which executes the binary defined by the target randomsimulation_tests
add_test(NAME randomsimulation_module_tests COMMAND randomsimulation_tests)

target_include_directories(randomsimulation_tests PUBLIC ${CMAKE_PROJECT_ROOT}/lib/googletest/googletest/include
                                                         ${CMAKE_PROJECT_ROOT}/lib/googletest/googlemock/include)

target_link_libraries(randomsimulation_tests ${CANDY_LIBS} testutils gtest_main gtest)

