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
# II. Invoking cmake
#
# Examples:
#
#  Generate a debug build:
#    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug path/to/sources
#
#  Generate a sanitizer build:
#    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -DCANDY_ENABLE_LLVM_SANITIZER=1 path/to/sources
#  Run with symbolizer:
#    export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
#    ASAN_OPTIONS=symbolize=1 ./candy ...
#
#  Generate a fuzzer build:
#    cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_C_COMPILER=afl-gcc -DCMAKE_CXX_COMPILER=afl-g++ -DCMAKE_BUILD_TYPE=Debug path/to/sources
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
cmake_minimum_required(VERSION 3.5.1)

# Temporary solution for shared lib
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)

### Options

# Enable LLVM coverage
option(CANDY_ENABLE_COVERAGE "Enable coverage analysis" OFF)

# Enable LLVM address sanitizer
option(CANDY_ENABLE_LLVM_SANITIZER "Enable LLVM address sanitizer" OFF)

# Enable LLVM undefined-behavior, address and leak sanitizer
option(CANDY_ENABLE_LLVM_SANITIZER_FULL "Enable LLVM address, memory and undefined-behavior sanitizer" OFF)

# Enable address sanitizer
option(CANDY_ENABLE_ADDRESS_SANITIZER "Just enable address sanitizer" OFF)

# Enable -march=native - if set to OFF, at least SSE 4.1 will still be enabled.
option(CANDY_ENABLE_NATIVEARCH "Enable -march=native" OFF)

option(CANDY_ENABLE_STAREXEC_ARCH "Optimize for StarExec" OFF)

# Enable Sonification
option(CANDY_ENABLE_SONIFICATION "Enable the Glucose sonification" OFF)

# Enable Static Linking
option(CANDY_ENABLE_STATIC_LINKING "Enable static linking" OFF)

# Build candy as a shared library
option(CANDY_BUILD_SHARED_LIB "Build candy as a shared library (experimental)" OFF)

# Enable Link Time Optimization in Release Builds
option(CANDY_ENABLE_LTO "Enable linking time optimization" ON)

# Enable checked casts in debug mode
option(CANDY_ENABLE_DEBUG_CHECKEDCASTS "Enable checked casts in debug mode" ON)

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

### Platform feature checks
INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(getrlimit HAVE_GETRLIMIT)
CHECK_FUNCTION_EXISTS(setrlimit HAVE_SETRLIMIT)

if(HAVE_GETRLIMIT AND HAVE_SETRLIMIT)
  add_definitions(-D CANDY_HAVE_RLIMIT)
endif()


### Compiler flags

if(MSVC)
  set(CMAKE_CXX_FLAGS "/W3 /D __STDC_LIMIT_MACROS /D __STDC_FORMAT_MACROS /EHsc /F 134217728")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /D NDEBUG /Ox")
else()
  set(CMAKE_CXX_FLAGS "-std=c++11 -pthread -Wall -Wno-parentheses -D __STDC_LIMIT_MACROS -D __STDC_FORMAT_MACROS")
  #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-inline -Og")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-inline -g -O0 -DSF_VISIBILITY -fvisibility=hidden -fno-strict-aliasing -Wall")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -Wall -O2 -g")
  if (CANDY_ENABLE_LTO) 
    if( CMAKE_BUILD_TYPE MATCHES Release AND GXX_HAS_LTO_FLAG )
      find_program(CMAKE_GCC_AR NAMES ${_CMAKE_TOOLCHAIN_PREFIX}gcc-ar${_CMAKE_TOOLCHAIN_SUFFIX} HINTS ${_CMAKE_TOOLCHAIN_LOCATION})
      find_program(CMAKE_GCC_NM NAMES ${_CMAKE_TOOLCHAIN_PREFIX}gcc-nm HINTS ${_CMAKE_TOOLCHAIN_LOCATION})
      find_program(CMAKE_GCC_RANLIB NAMES ${_CMAKE_TOOLCHAIN_PREFIX}gcc-ranlib HINTS ${_CMAKE_TOOLCHAIN_LOCATION})

      if( CMAKE_GCC_AR AND CMAKE_GCC_NM AND CMAKE_GCC_RANLIB )
        set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto -fno-fat-lto-objects" )
        set( CMAKE_AR "${CMAKE_GCC_AR}" )
        set( CMAKE_NM "${CMAKE_GCC_NM}" )
        set( CMAKE_RANLIB "${CMAKE_GCC_RANLIB}" )
      else()
        message( WARNING "GCC indicates LTO support, but binutils wrappers could not be found. Disabling LTO." )
      endif()
    endif()
  endif()

  if(CANDY_BUILD_SHARED_LIB)
    add_definitions(-fPIC)
  endif()
endif()

if(CANDY_ENABLE_DEBUG_CHECKEDCASTS)
  add_definitions(-D DEBUG_CHECK_CASTS)
endif()

if(CANDY_ENABLE_COVERAGE)
  if(MSVC)
    message(FATAL_ERROR "Coverage not supported for MSVC.")
  endif()
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
  SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
  SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} --coverage")
endif()

if(CANDY_ENABLE_LLVM_SANITIZER)
  if(MSVC)
    message(FATAL_ERROR "CANDY_ENABLE_LLVM_SANITIZER is not yet supported for MSVC")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address,undefined")
endif()

if(CANDY_ENABLE_LLVM_SANITIZER_FULL)
  if(MSVC)
    message(FATAL_ERROR "CANDY_ENABLE_LLVM_SANITIZER_FULL is not yet supported for MSVC")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address,memory,undefined")
endif()

if(CANDY_ENABLE_ADDRESS_SANITIZER)
  if(MSVC)
    message(FATAL_ERROR "CANDY_ENABLE_ADDRESS_SANITIZER is not yet supported for MSVC")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

if(CANDY_DISABLE_RTTI)
  message("WARNING: Use CANDY_DISABLE_RTTI only for the most optimized builds, since the test system relies on it")
  add_definitions(-DCANDY_HAS_NO_RTTI)
  if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
  endif()
endif()

if(CANDY_ENABLE_STATIC_LINKING)
  message("Static linking is enabled.")
  SET(BUILD_SHARED_LIBRARIES OFF)

  if (NOT MSVC)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    SET(CMAKE_EXE_LINKER_FLAGS "-static")
  endif()
endif()

if (CANDY_ENABLE_NATIVEARCH)
  message("Compiling for the processor architecture of this machine.")
  if(MSVC)
    message(FATAL_ERROR "CANDY_ENABLE_NATIVEARCH is not yet supported for MSVC")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
elseif (CANDY_ENABLE_STAREXEC_ARCH)
  message("Compiling for the processor architecture of StarExec Cluster.")
  if(MSVC)
    message(FATAL_ERROR "CANDY_ENABLE_STAREXEC_ARCH is not supported for MSVC")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=corei7-avx")
else ()
  message("Compiling for processors supporting at least SSE 4.1.")
  if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.1")
  endif()
endif ()

### Additional Preprocessor Defintions

if(CANDY_ENABLE_SONIFICATION)
  if (MSVC)
    message(FATAL_ERROR "CANDY_ENABLE_SONIFICATION is not yet supported for MSVC")
  endif()
  add_definitions(-DSONIFICATION)
endif()

### Find libraries
# set zlib for windows with mingw
if (MINGW)
	set(ZLIB_INCLUDE_DIR  "C:/mingw-w64/x86_64-8.1.0-posix-seh-rt_v6-rev0/mingw64/x86_64-w64-mingw32/include")
	set(ZLIB_LIBRARY "C:/mingw-w64/x86_64-8.1.0-posix-seh-rt_v6-rev0/mingw64/x86_64-w64-mingw32/lib/libz.a")
endif (MINGW)
# end mingw config

find_package(ZLIB)
if (${ZLIB_FOUND})
   include_directories(${ZLIB_INCLUDE_DIRS})
else (${ZLIB_FOUND})
   message(FATAL_ERROR "Could not find zlib.")
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
include_directories(${PROJECT_SOURCE_DIR}/lib/oscpack)

set(GTEST_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/lib/googletest/googletest/include" "${PROJECT_SOURCE_DIR}/lib/googletest/googlemock/include")

### Add subdirectories (containing CMakeLists with further, directory-specific build instructions)

add_subdirectory(src)

set(CANDY_OBJECTS
  $<TARGET_OBJECTS:gates>
  $<TARGET_OBJECTS:randomsimulation>
  $<TARGET_OBJECTS:rsar>
  $<TARGET_OBJECTS:ipasir>
  $<TARGET_OBJECTS:minimizer>
  $<TARGET_OBJECTS:core>
  $<TARGET_OBJECTS:systems>
  $<TARGET_OBJECTS:simplification>
  $<TARGET_OBJECTS:sonification>
  $<TARGET_OBJECTS:frontend>
  $<TARGET_OBJECTS:utils>
  $<TARGET_OBJECTS:teexgraph>)

### OSCPack
link_directories(${CMAKE_BINARY_DIR}/lib/oscpack)

set(CANDY_LIBS ${ZLIB_LIBRARIES} oscpack)

add_subdirectory(lib)
add_subdirectory(testsrc)

### Define and link target executables and libraries
set(candylib_build_mode STATIC)
if(CANDY_BUILD_SHARED_LIB)
  set(candylib_build_mode SHARED)
endif()

add_library(candylib ${candylib_build_mode} ${CANDY_OBJECTS})
target_link_libraries(candylib ${CANDY_LIBS})

add_executable(candy src/candy/Main.cc $<TARGET_OBJECTS:frontend>)
add_executable(cnf2aig src/candy/cnf2aig.cc $<TARGET_OBJECTS:frontend>)
add_executable(fuzz_ipasir src/candy/FuzzIpasir.cc $<TARGET_OBJECTS:frontend>)

candy_executable_customizations(candy)
FIND_LIBRARY (PSAPI Psapi)
if (PSAPI)
	target_link_libraries(candy candylib ${PSAPI})
	target_link_libraries(cnf2aig candylib ${PSAPI})
	target_link_libraries(fuzz_ipasir candylib ${PSAPI})
ELSE()
	target_link_libraries(candy candylib)
	target_link_libraries(cnf2aig candylib)
	target_link_libraries(fuzz_ipasir candylib)
ENDIF()



