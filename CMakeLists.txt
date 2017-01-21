project(Candy)
cmake_minimum_required(VERSION 3.7)

option(ENABLE_LLVM_COVERAGE "Enable coverage analysis" OFF)
option(ENSURE_DSYM "Ensure that .dSYM files are generated" OFF)

### Compiler flags

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wno-parentheses -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS  -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-rtti -fno-inline -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -Wall -flto -fno-rtti -O3")


if(ENABLE_LLVM_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
endif()

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
add_subdirectory(randomsimulation)

### Define and link target executables

set(CANDY_LIBS utils core simp gates randomsimulation ${ZLIB_LIBRARIES})

add_executable(candy Main.cc)
target_link_libraries(candy ${CANDY_LIBS})

if (ENSURE_DSYM)
  add_custom_command(TARGET candy POST_BUILD COMMAND dsymutil candy)
endif()




# The tests executable target is defined in test/
add_subdirectory(test)


