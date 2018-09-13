add_executable(candy_fuzz
    candy_fuzz.cc
    ${CANDY_OBJECTS})

candy_executable_customizations(candy_fuzz)

ADD_LIBRARY(minisat STATIC IMPORTED)
SET_TARGET_PROPERTIES(candy_fuzz minisat PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/minisat/build/release/lib/libminisat.a)

link_directories(${CMAKE_BINARY_DIR}/lib/minisat/build/release/lib)

target_include_directories(candy_fuzz PUBLIC ${PROJECT_SOURCE_DIR}/lib/minisat/minisat/core)

target_link_libraries(candy_fuzz ${CANDY_LIBS} minisat)
