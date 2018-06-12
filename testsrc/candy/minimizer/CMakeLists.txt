add_executable(minimizer_tests
    MinimizerTests.cc
    ${CANDY_OBJECTS}
    $<TARGET_OBJECTS:testutils>)
candy_executable_customizations(minimizer_tests)

target_include_directories(minimizer_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                              ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)

target_include_directories(minimizer_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)

target_link_libraries(minimizer_tests ${CANDY_LIBS} gtest_main gtest)

# add a ctest test named gates_module_tests which executes the binary defined by the target gates_tests
add_test(NAME minimizer_module_tests COMMAND minimizer_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

