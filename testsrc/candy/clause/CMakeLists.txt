add_executable(clause_tests
  Main.cc)
candy_executable_customizations(clause_tests)

target_include_directories(clause_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                        ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)

target_link_libraries(clause_tests ${CANDY_LIBS} gtest_main gtest)

# add a ctest test named clause_module_tests which executes the binary defined by the target clause_tests
add_test(NAME clause_module_tests COMMAND clause_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
