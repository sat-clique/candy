# Candy CMake files
#
#
# I. Building
#
# 1. Create a directory X, e.g. build/ and change to X
# 2. Invoke cmake (see below)
# 3. Run make
#
# The candy binary will be generated in X/, while the test binaries are placed in
# X/testsrc/$TESTMODULENAME (e.g. testsrc/gates/gates_tests). For running the tests,
# simply run ctest (if you wish to see stdout, run ctest --output-on-error (showing
# the test output only when it fails) or ctest --verbose
#
#
# To build the documentation, run 
#   make doxygen
# or
#   cmake --build . --target doxygen
#
# The documentation is generated in doc/ within your build directory. 
#
# II. Invoking cmake
#
# Examples:
#
#  Generate a debug build:
#    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug path/to/sources
#
#  Generate a release build:
#    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release path/to/sources
#
#  Generate an Xcode IDE project (sources still remain at their original place):
#    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -G Xcode path/to/sources
#
#  Generate an Eclipse IDE project (sources still remain at their original place):
#    cmake -G"Eclipse CDT4 - Unix Makefiles"
#  (see https://cmake.org/Wiki/Eclipse_CDT4_Generator)
#
#
#
# III. Project structure of candy
#
#  lib/         external libraries
#  src/         source code
#    candy/     candy source code
#  testsrc/     test source code
#    candy/     candy test source code
#      ...      one directory for each module under test



project(Candy)
cmake_minimum_required(VERSION 3.5)


### Options

# Enable LLVM coverage
option(ENABLE_LLVM_COVERAGE "Enable coverage analysis" OFF)

# Enable LLVM address and leak sanitizer
option(ENABLE_LLVM_SANITIZER "Enable LLVM address sanitizer" OFF)

# Enable dSYM hack (only relevant for macOS builds)
option(ENSURE_DSYM "Ensure that .dSYM files are generated" OFF)

# Disable RTTI and defines CANDY_HAS_NO_RTTI.
# WARNING: Enable this only AFTER having thoroughly tested the code. In some places
# RTTI is needed for testing (and only in places where it is also safe to use a
# reinterpret_cast - testing interfaces which document the single allowed dynamic type
# of the pointer getting casted).
# This changes the code to use reinterpret_cast instead of dynamic_cast via
# preprocessor directives.
#
# TODO: Remove this option as soon as a dynamic_cast cannot safely be replaced
# by a reinterpret_cast.
option(CANDY_DISABLE_RTTI OFF "Disable RTTI and defines CANDY_HAS_NO_RTTI. Use with caution.")


### Using ctest as a test

enable_testing()


### Add doxygen targets
add_subdirectory(doc)

### Compiler flags

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wno-parentheses -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS  -march=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-inline -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -Wall -flto -O3")

if(ENABLE_LLVM_SANITIZER)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

if(CANDY_DISABLE_RTTI)
  message("WARNING: Use CANDY_DISABLE_RTTI only for the most optimized builds, since the test system relies on it")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -DCANDY_HAS_NO_RTTI")
endif()

### Find libraries

find_package(ZLIB)
if (${ZLIB_FOUND})
   include_directories(${ZLIB_INCLUDE_DIRS})
else (${ZLIB_FOUND})
   message("Could not find zlib.")
endif (${ZLIB_FOUND})


### Custom functions


# This function is responsible for executable post-processing, e.g. running dsymutil.
function(candy_executable_customizations targetname)
  if (ENSURE_DSYM)
    add_custom_command(TARGET ${targetname} POST_BUILD COMMAND dsymutil Debug/${targetname})
  endif()
endfunction()



### Set up include directories

include_directories(${PROJECT_SOURCE_DIR}/src)
# for refactoring purposes, TODO: use e.g. candy/core/SolverTypes.h instead of core/SolverTypes.h
include_directories(${PROJECT_SOURCE_DIR}/src/candy)

set(GTEST_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include" "${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include")


### Add subdirectories (containing CMakeLists with further, directory-specific build instructions)

add_subdirectory(src)
set(CANDY_LIBS utils core simp gates randomsimulation rsar ${ZLIB_LIBRARIES})

add_subdirectory(lib)
add_subdirectory(testsrc)

### Define and link target executables and libraries

add_executable(candy src/candy/Main.cc)
candy_executable_customizations(candy)
target_link_libraries(candy ${CANDY_LIBS})


