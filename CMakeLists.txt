project(Candy)
cmake_minimum_required(VERSION 3.7)

### Options

option(ENABLE_LLVM_COVERAGE "Enable coverage analysis" OFF)
option(ENSURE_DSYM "Ensure that .dSYM files are generated" OFF)

### Compiler flags

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wno-parentheses -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS  -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-rtti -fno-inline -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -Wall -flto -fno-rtti -O3")

### Find libraries

find_package(ZLIB)
if (${ZLIB_FOUND})
   include_directories(${ZLIB_INCLUDE_DIRS})
else (${ZLIB_FOUND})
   message("Could not find zlib.")
endif (${ZLIB_FOUND})


### Custom functions

function(candy_executable_customizations targetname)
  if (ENSURE_DSYM)
# TODO: Switch "Debug/" on/off...
    add_custom_command(TARGET ${targetname} POST_BUILD COMMAND dsymutil Debug/${targetname})
  endif()
endfunction()



### Set up include directories

include_directories(${PROJECT_SOURCE_DIR})
# for refactoring purposes
include_directories(${PROJECT_SOURCE_DIR}/src/candy)

### Add subdirectories
# In each subdirectory X, we define a library named X (e.g. the "core" library.)

add_subdirectory(src)
add_subdirectory(lib)

set(GTEST_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include" "${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include")


### Define and link target executables

set(CANDY_LIBS utils core simp gates randomsimulation ${ZLIB_LIBRARIES})

add_executable(candy Main.cc)
candy_executable_customizations(candy)

target_link_libraries(candy ${CANDY_LIBS})

# adding testsrc late because it needs ${CANDY_LIBS} to be defined
add_subdirectory(testsrc)
