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

add_executable(candy_parser_fuzz
    candy_parser_fuzz.cc
    ${CANDY_OBJECTS})

add_executable(candy_ca_fuzz
    candy_ca_fuzz.cc
    ${CANDY_OBJECTS})

candy_executable_customizations(candy_fuzz)
candy_executable_customizations(candy_proof_fuzz)
candy_executable_customizations(candy_parser_fuzz)
candy_executable_customizations(candy_ca_fuzz)

include_directories(${PROJECT_SOURCE_DIR}/lib/minisat)
include_directories(${PROJECT_SOURCE_DIR}/lib/drat-trim)

target_link_libraries(candy_fuzz ${CANDY_LIBS} minisat-lib-static)
target_link_libraries(candy_proof_fuzz ${CANDY_LIBS} minisat-lib-static dt)
target_link_libraries(candy_parser_fuzz ${CANDY_LIBS})
target_link_libraries(candy_ca_fuzz ${CANDY_LIBS})

# Make sure that the fuzz-test seed CNF problems are copied to the build directory
file(GLOB TEST_CNF_PROBLEMS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "seeds/*.cnf")
message("Test CNF files: ${TEST_CNF_PROBLEMS}")

foreach(CNF_PROBLEM ${TEST_CNF_PROBLEMS})
    configure_file(${CNF_PROBLEM} ${CNF_PROBLEM} COPYONLY)
endforeach()
