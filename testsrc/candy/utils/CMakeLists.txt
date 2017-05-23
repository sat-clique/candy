add_executable(utils_tests
    StringUtilsTests.cc
    CNFProblemTests.cc
)

target_include_directories(utils_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)
target_include_directories(utils_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc/candy)

candy_executable_customizations(utils_tests)

# add a ctest test named randomsimulation_module_tests which executes the binary defined by the target randomsimulation_tests
add_test(NAME utils_module_tests COMMAND utils_tests)

target_include_directories(utils_tests PUBLIC ${CMAKE_PROJECT_ROOT}/lib/googletest/googletest/include
                                             ${CMAKE_PROJECT_ROOT}/lib/googletest/googlemock/include)
target_link_libraries(utils_tests ${CANDY_LIBS} testutils gtest_main gtest gmock)

