add_executable(ipasir_tests
    IpasirTests.cc
    ${CANDY_OBJECTS}
    $<TARGET_OBJECTS:testutils>)
candy_executable_customizations(ipasir_tests)

target_include_directories(ipasir_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                              ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)

target_include_directories(ipasir_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)

target_link_libraries(ipasir_tests ${CANDY_LIBS} gtest_main gtest)

# add a ctest test named gates_module_tests which executes the binary defined by the target gates_tests
add_test(NAME ipasir_module_tests COMMAND ipasir_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

