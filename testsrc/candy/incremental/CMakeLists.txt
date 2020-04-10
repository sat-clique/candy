add_executable(incremental_tests
    IncrementalTests.cc
    ${CANDY_OBJECTS}
    $<TARGET_OBJECTS:testutils>)
candy_executable_customizations(incremental_tests)

target_include_directories(incremental_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                              ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)

target_include_directories(incremental_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)

target_link_libraries(incremental_tests ${CANDY_LIBS} gtest_main gtest)

add_test(NAME incremental_module_tests COMMAND incremental_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB TEST_CNF_PROBLEMS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "problems/*.cnf")
message("Test CNF files: ${TEST_CNF_PROBLEMS}")

foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
    configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
endforeach()