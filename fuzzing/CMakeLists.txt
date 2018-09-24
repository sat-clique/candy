add_executable(candy_fuzz
    candy_fuzz.cc
    util.h
    util.cc
    ${CANDY_OBJECTS})

add_executable(candy_proof_fuzz
    candy_proof_fuzz.cc
    util.h
    util.cc
    ${CANDY_OBJECTS})

candy_executable_customizations(candy_fuzz)

target_include_directories(candy_fuzz PUBLIC ${PROJECT_SOURCE_DIR}/lib/minisat)

target_include_directories(candy_proof_fuzz PUBLIC ${PROJECT_SOURCE_DIR}/lib/minisat)
target_include_directories(candy_proof_fuzz PUBLIC ${PROJECT_SOURCE_DIR}/lib/drat-trim)

add_library(minisat STATIC IMPORTED)
set_target_properties(minisat PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/minisat/libminisat.a)

add_library(dt STATIC IMPORTED)
set_target_properties(dt PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/drat-trim/libdt.a)

link_directories(${CMAKE_BINARY_DIR}/lib/minisat)
link_directories(${CMAKE_BINARY_DIR}/lib/drat-trim)

target_link_libraries(candy_fuzz ${CANDY_LIBS} minisat)
target_link_libraries(candy_proof_fuzz ${CANDY_LIBS} minisat dt)

# Make sure that the fuzz-test seed CNF problems are copied to the build directory
file(GLOB TEST_CNF_PROBLEMS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "seeds/*.cnf")
message("Test CNF files: ${TEST_CNF_PROBLEMS}")

foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
    configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
endforeach()
