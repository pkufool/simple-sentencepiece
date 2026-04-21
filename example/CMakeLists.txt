# -----------------------------------------------------------------------------
# Example CMakeLists.txt showing how to integrate simple-sentencepiece into
# an existing C++ project.
#
# Three integration strategies are provided below. Pick ONE by setting
#   -DSSPIECE_INTEGRATION=<subdir|fetch|installed>
# on the cmake command line. The default is "subdir" which simply points at
# the parent repository checkout (../).
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.14)
project(ssentencepiece_demo CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SSPIECE_INTEGRATION "subdir" CACHE STRING
    "How to obtain ssentencepiece: subdir | fetch | installed")

# -----------------------------------------------------------------------------
# Strategy A: add_subdirectory of the in-tree checkout (default).
#
# Useful when ssentencepiece lives next to your project as a git submodule
# or vendored copy. This example sits inside the repo, so we point one level
# up. The `SBPE_BUILD_PYTHON=OFF` switch is what makes the build lean: the
# C++ core has *zero* third-party runtime deps (no protobuf!).
# -----------------------------------------------------------------------------
if(SSPIECE_INTEGRATION STREQUAL "subdir")
  message(STATUS "ssentencepiece integration: add_subdirectory(..)")
  set(SBPE_BUILD_PYTHON OFF CACHE BOOL "" FORCE)
  set(SBPE_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)
  # The parent project expects `#include "ssentencepiece/csrc/..."` to
  # resolve against the repo root, so expose it as an include path for
  # every target built here (including the library itself).
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/..
                   ${CMAKE_CURRENT_BINARY_DIR}/_ssentencepiece_build)

# -----------------------------------------------------------------------------
# Strategy B: FetchContent from GitHub — no submodule, no manual checkout.
# -----------------------------------------------------------------------------
elseif(SSPIECE_INTEGRATION STREQUAL "fetch")
  message(STATUS "ssentencepiece integration: FetchContent")
  include(FetchContent)
  set(SBPE_BUILD_PYTHON OFF CACHE BOOL "" FORCE)
  set(SBPE_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS  OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    ssentencepiece
    GIT_REPOSITORY https://github.com/pkufool/simple-sentencepiece.git
    GIT_TAG        master
  )
  FetchContent_MakeAvailable(ssentencepiece)

# -----------------------------------------------------------------------------
# Strategy C: consume an already-installed copy (header + static/shared lib).
#
# After `cmake --install` of the parent project you'll have:
#   <prefix>/ssentencepiece/csrc/ssentencepiece.h
#   <prefix>/libssentencepiece_core.{a,so}
# Point SSPIECE_ROOT at that prefix.
# -----------------------------------------------------------------------------
elseif(SSPIECE_INTEGRATION STREQUAL "installed")
  message(STATUS "ssentencepiece integration: installed prefix")
  if(NOT DEFINED SSPIECE_ROOT)
    message(FATAL_ERROR
      "Please pass -DSSPIECE_ROOT=/path/to/install/prefix")
  endif()
  add_library(ssentencepiece_core UNKNOWN IMPORTED)
  find_library(_sspiece_lib
    NAMES ssentencepiece_core
    PATHS ${SSPIECE_ROOT} ${SSPIECE_ROOT}/lib
    REQUIRED)
  set_target_properties(ssentencepiece_core PROPERTIES
    IMPORTED_LOCATION             "${_sspiece_lib}"
    INTERFACE_INCLUDE_DIRECTORIES "${SSPIECE_ROOT}")

else()
  message(FATAL_ERROR
    "Unknown SSPIECE_INTEGRATION='${SSPIECE_INTEGRATION}'. "
    "Use one of: subdir | fetch | installed.")
endif()

# -----------------------------------------------------------------------------
# Your own target — link against ssentencepiece_core, done.
# -----------------------------------------------------------------------------
add_executable(ssentencepiece_demo demo.cc)
target_link_libraries(ssentencepiece_demo PRIVATE ssentencepiece_core)

# pthread is needed on POSIX for the built-in ThreadPool.
if(NOT WIN32)
  find_package(Threads REQUIRED)
  target_link_libraries(ssentencepiece_demo PRIVATE Threads::Threads)
endif()
