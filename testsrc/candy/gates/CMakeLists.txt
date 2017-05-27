add_executable(gates_tests
    GateDFSTraversalTests.cc)
candy_executable_customizations(gates_tests)

target_include_directories(gates_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                              ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)

target_include_directories(gates_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)
target_include_directories(gates_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc/candy)

target_link_libraries(gates_tests ${CANDY_LIBS} testutils gtest_main gtest)

# add a ctest test named gates_module_tests which executes the binary defined by the target gates_tests
add_test(NAME gates_module_tests COMMAND gates_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})


# Make sure that the test CNF problems are copied to the build directory
file(GLOB TEST_CNF_PROBLEMS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cnf")
message("Test CNF files: ${TEST_CNF_PROBLEMS}")

foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
    configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
endforeach()
