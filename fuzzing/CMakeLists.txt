add_executable(candy_fuzz
    candy_fuzz.cc
    ${CANDY_OBJECTS})

candy_executable_customizations(candy_fuzz)

target_include_directories(candy_fuzz PUBLIC ${PROJECT_SOURCE_DIR}/lib/minisat)

add_library(minisat STATIC IMPORTED)
set_target_properties(minisat PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/minisat/libminisat.a)

link_directories(${CMAKE_BINARY_DIR}/lib/minisat)

target_link_libraries(candy_fuzz ${CANDY_LIBS} minisat)

# Make sure that the fuzz-test seed CNF problems are copied to the build directory
file(GLOB TEST_CNF_PROBLEMS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "seeds/*.cnf")
message("Test CNF files: ${TEST_CNF_PROBLEMS}")

foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
    configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
endforeach()
