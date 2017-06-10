add_library(testutils OBJECT
    TestGateStructure.h
    TestGateStructure.cc
    TestUtils.h
    TestUtils.cc)

add_executable(testutils_tests
    TestGateStructureTests.cc
    ${CANDY_OBJECTS}
    $<TARGET_OBJECTS:testutils>)

candy_executable_customizations(testutils_tests)

add_test(NAME testutils_module_tests COMMAND testutils_tests)

target_include_directories(testutils_tests PUBLIC ${CMAKE_PROJECT_ROOT}/lib/googletest/googletest/include
                                                  ${CMAKE_PROJECT_ROOT}/lib/googletest/googlemock/include)
target_link_libraries(testutils_tests ${CANDY_LIBS} gtest_main gtest)
