add_executable(rsil_tests
	TestUtils.h
        BranchingHeuristicsTests.cc
	RSARHeuristicsFilterTests.cc
        ImplicitLearningAdviceTests.cc)

candy_executable_customizations(rsil_tests)

target_include_directories(rsil_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)
target_include_directories(rsil_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc/candy)


# add a ctest test named randomsimulation_module_tests which executes the binary defined by the target rsil_tests
add_test(NAME rsil_module_tests COMMAND rsil_tests)

target_include_directories(rsil_tests PUBLIC ${CMAKE_PROJECT_ROOT}/lib/googletest/googletest/include
                                             ${CMAKE_PROJECT_ROOT}/lib/googletest/googlemock/include)

target_link_libraries(rsil_tests ${CANDY_LIBS} testutils gtest_main gtest gmock)

file(GLOB TEST_CNF_PROBLEMS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "problems/*.cnf")
message("Test CNF files: ${TEST_CNF_PROBLEMS}")

foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
endforeach()
