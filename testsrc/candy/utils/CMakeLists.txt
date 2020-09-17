add_executable(utils_tests
    CNFProblemTests.cc
    StampTests.cc
    ${CANDY_OBJECTS}
    $<TARGET_OBJECTS:testutils>)

target_include_directories(utils_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)

candy_executable_customizations(utils_tests)

# add a ctest test named randomsimulation_module_tests which executes the binary defined by the target randomsimulation_tests
add_test(NAME utils_module_tests COMMAND utils_tests)

target_include_directories(utils_tests PUBLIC ${CMAKE_PROJECT_ROOT}/lib/googletest/googletest/include
                                             ${CMAKE_PROJECT_ROOT}/lib/googletest/googlemock/include)
target_link_libraries(utils_tests ${CANDY_LIBS} gtest_main gtest gmock)

