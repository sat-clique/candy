add_executable(rsar_tests Main.cc
			  RefinementStateTests.cc)


candy_executable_customizations(rsar_tests)

# add a ctest test named randomsimulation_module_tests which executes the binary defined by the target randomsimulation_tests
add_test(NAME rsar_module_tests COMMAND rsar_tests)

target_include_directories(rsar_tests PUBLIC ${CMAKE_PROJECT_ROOT}/lib/googletest/googletest/include
                                             ${CMAKE_PROJECT_ROOT}/lib/googletest/googlemock/include)
target_link_libraries(rsar_tests ${CANDY_LIBS} gtest)

