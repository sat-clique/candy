add_subdirectory(googletest/googletest)

add_executable(tests
    Main.cc)

target_include_directories(tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include
                                        ${CMAKE_CURRENT_SOURCE_DIR}/googletest/googlemock/include)

target_link_libraries(tests ${CANDY_LIBS} gtest)


# Make sure that the test CNF problems are copied to the build directory
file(GLOB TEST_CNF_PROBLEMS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*.cnf")
message("Test CNF files: ${TEST_CNF_PROBLEMS}")

foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
    configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
endforeach()
