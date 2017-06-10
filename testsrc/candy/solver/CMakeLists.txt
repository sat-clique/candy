add_executable(solver_tests
  Main.cc
  ${CANDY_OBJECTS})
candy_executable_customizations(solver_tests)

target_include_directories(solver_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                        ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)

target_link_libraries(solver_tests ${CANDY_LIBS} gtest_main gtest)

# add a ctest test named solver_module_tests which executes the binary defined by the target solver_tests
add_test(NAME solver_module_tests COMMAND solver_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
