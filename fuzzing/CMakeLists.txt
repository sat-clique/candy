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
candy_executable_customizations(candy_proof_fuzz)

include_directories(${PROJECT_SOURCE_DIR}/lib/minisat)
include_directories(${PROJECT_SOURCE_DIR}/lib/drat-trim)

link_directories(${CMAKE_BINARY_DIR}/lib/minisat)
link_directories(${CMAKE_BINARY_DIR}/lib/drat-trim)

add_library(minisat STATIC IMPORTED)
add_library(dt STATIC IMPORTED)

find_library(MINISAT_LIBRARY NAMES minisat PATHS "${CMAKE_BINARY_DIR}/lib/minisat")
set_target_properties(minisat PROPERTIES IMPORTED_LOCATION ${MINISAT_LIBRARY})

find_library(DT_LIBRARY NAMES dt PATHS "${CMAKE_BINARY_DIR}/lib/drat-trim")
set_target_properties(dt PROPERTIES IMPORTED_LOCATION ${DT_LIBRARY})

target_link_libraries(candy_fuzz ${CANDY_LIBS} minisat)
target_link_libraries(candy_proof_fuzz ${CANDY_LIBS} minisat dt)

# Make sure that the fuzz-test seed CNF problems are copied to the build directory
file(GLOB TEST_CNF_PROBLEMS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "seeds/*.cnf")
message("Test CNF files: ${TEST_CNF_PROBLEMS}")

foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
    configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
endforeach()
