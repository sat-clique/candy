add_executable(integration_tests
    IntegrationTests.cc
    ${CANDY_OBJECTS}
    $<TARGET_OBJECTS:testutils>)
candy_executable_customizations(integration_tests)

target_include_directories(integration_tests PUBLIC ${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include
                                              ${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include)

target_include_directories(integration_tests PRIVATE ${PROJECT_SOURCE_DIR}/testsrc)

target_link_libraries(integration_tests ${CANDY_LIBS} gtest_main gtest)

add_test(NAME integration_module_tests COMMAND integration_tests WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

#file(GLOB TEST_CNF_PROBLEMS "${PROJECT_SOURCE_DIR}/data/cnf/*.cnf")
file(COPY "${PROJECT_SOURCE_DIR}/data/cnf" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
#    configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
#endforeach()