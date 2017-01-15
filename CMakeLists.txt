project(Candy)
cmake_minimum_required(VERSION 3.7)

### Compiler flags

set (CMAKE_CXX_FLAGS "-std=c++11")

### Find libraries

find_package(ZLIB)
if (${ZLIB_FOUND})
   message("Found ZLIB: " ${ZLIB_LIBRARIES})
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

add_executable(candy Main.cc)
target_link_libraries(candy utils core simp gates ${ZLIB_LIBRARIES})
