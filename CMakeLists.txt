project(Candy)
cmake_minimum_required(VERSION 3.7)

### Compiler flags

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wno-parentheses -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS")
# todo: -flto linker flag

### Find libraries

find_package(ZLIB)
if (${ZLIB_FOUND})
   include_directories(${ZLIB_INCLUDE_DIRS})
else (${ZLIB_FOUND})
   message("Could not find zlib.")
endif (${ZLIB_FOUND})

### Set up include directories

include_directories(${PROJECT_SOURCE_DIR})

### Add subdirectories
# In each subdirectory X, we define a library named X (e.g. the "core" library.)

add_subdirectory(utils)
add_subdirectory(core)
add_subdirectory(simp)
add_subdirectory(gates)


### Define and link target executables

set(CANDY_LIBS utils core simp gates ${ZLIB_LIBRARIES})

add_executable(candy Main.cc)
target_link_libraries(candy ${CANDY_LIBS})

# The tests executable target is defined in test/
add_subdirectory(test)


