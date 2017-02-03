add_executable(rsar_tests Main.cc
			  SolverMock.cc
			  SolverMock.h
			  SolverMockTests.cc
			  TestUtils.h
			  TestUtils.cc
			  HeuristicsMock.h
			  HeuristicsMock.cc
			  RefinementTests.cc
			  ApproximationStateTests.cc
			  ARSolverTests.cc)


candy_executable_customizations(rsar_tests)

# add a ctest test named randomsimulation_module_tests which executes the binary defined by the target randomsimulation_tests
add_test(NAME rsar_module_tests COMMAND rsar_tests)

target_include_directories(rsar_tests PUBLIC ${CMAKE_PROJECT_ROOT}/lib/googletest/googletest/include
                                             ${CMAKE_PROJECT_ROOT}/lib/googletest/googlemock/include)
target_link_libraries(rsar_tests ${CANDY_LIBS} gtest gmock)

file(GLOB TEST_CNF_PROBLEMS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "problems/*.cnf")
message("Test CNF files: ${TEST_CNF_PROBLEMS}")

foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
    configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
endforeach()
