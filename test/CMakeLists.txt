add_subdirectory(googletest/googletest)

add_executable(tests
    Main.cc)

target_include_directories(tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include
                                        ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googlemock/include)

target_link_libraries(tests ${CANDY_LIBS} gtest)



add_executable(randomsimulation_tests
    randomsimulation/Main.cc
    randomsimulation/PartitionTests.cc
    randomsimulation/PropagationTests.cc
    randomsimulation/ClauseOrderTests.cc
    randomsimulation/TestGateStructure.h
    randomsimulation/TestGateStructure.cc
    randomsimulation/SimulationVectorTests.cc
    randomsimulation/TestUtils.cc
    randomsimulation/TestUtils.h
    randomsimulation/TestGateStructureTests.cc
    randomsimulation/RandomSimulatorTests.cc)

target_compile_options(randomsimulation_tests PRIVATE -Wextra -Weffc++)


target_include_directories(randomsimulation_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include
                                                         ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googlemock/include)

target_link_libraries(randomsimulation_tests ${CANDY_LIBS} gtest)

if (ENSURE_DSYM)
  add_custom_command(TARGET randomsimulation_tests POST_BUILD COMMAND dsymutil Debug/randomsimulation_tests)
endif()




# Make sure that the test CNF problems are copied to the build directory
file(GLOB TEST_CNF_PROBLEMS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cnf")
message("Test CNF files: ${TEST_CNF_PROBLEMS}")

foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
    configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
endforeach()
