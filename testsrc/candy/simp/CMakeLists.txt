add_executable(simp_tests
    SimpSolverTests.cc)

candy_executable_customizations(simp_tests)

add_test(NAME simp_module_tests COMMAND simp_tests)

target_include_directories(simp_tests PUBLIC ${CMAKE_PROJECT_ROOT}/lib/googletest/googletest/include
                                             ${CMAKE_PROJECT_ROOT}/lib/googletest/googlemock/include)
target_link_libraries(simp_tests ${CANDY_LIBS} gtest_main gtest)

