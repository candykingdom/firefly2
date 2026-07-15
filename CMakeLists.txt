cmake_minimum_required(VERSION 3.11)
if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED On)
set(CMAKE_CXX_EXTENSIONS Off)

project(firefly2 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake-fetch-content)
include(FetchContent)
include(fetch-content-shim)
include(ExternalProject)

FetchContent_Declare(googletest
                 GIT_REPOSITORY      https://github.com/google/googletest.git
                 GIT_TAG             main
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(fake-fast-led
                 GIT_REPOSITORY      https://github.com/ademuri/FakeFastLED.git
                 GIT_TAG             master
)
FetchContent_MakeAvailable(fake-fast-led)

# JSON parser for ReferenceVectorTest's reference-corpus comparison. Pinned
# tag; linked into the test targets only.
FetchContent_Declare(nlohmann_json
                 GIT_REPOSITORY      https://github.com/nlohmann/json.git
                 GIT_TAG             v3.11.3
                 GIT_SHALLOW         TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# -fno-sanitize-recover makes UBSan findings abort (and so fail the test run)
# instead of print-and-continue; without it the sanitizer is advisory-only.
set(SANITIZER_FLAGS
    "-fsanitize=address -fsanitize=undefined -fno-sanitize-recover=undefined")
# Set appropriate warning levels and include debug symbols.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++14 -g ${SANITIZER_FLAGS}")
set (CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS_DEBUG} ${SANITIZER_FLAGS}")

# Generate the file needed for YCM integration
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

file(GLOB LIBRARY_SOURCES
  "src/generic/*.cpp"
  "src/generic/FakeFastLED/*.cpp"
  "lib/color/*.cpp"
  "lib/debug/*.cpp"
  "lib/device/*.cpp"
  "lib/effect/*.cpp"
  "lib/fake-led-manager/*.cpp"
  "lib/fake-radio/*.cpp"
  "lib/led_manager/*.cpp"
  "lib/math/*.cpp"
  "lib/radio/*.cpp"
  "lib/types/*.cpp"
)
add_library(generic ${LIBRARY_SOURCES})
target_include_directories(generic PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/generic>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/generic/FakeFastLED>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/battery>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/color>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/debug>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/device>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/effect>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/fake-led-manager>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/fake-radio>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/led_manager>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/math>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/network>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/radio>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/types>
  $<INSTALL_INTERFACE:./src/generic>
  $<INSTALL_INTERFACE:./src/generic/FakeFastLED>
  $<INSTALL_INTERFACE:./lib/debug>
  $<INSTALL_INTERFACE:./lib/fake-led-manager>
  $<INSTALL_INTERFACE:./lib/fake-radio>
  $<INSTALL_INTERFACE:./lib/led_manager>
  $<INSTALL_INTERFACE:./lib/effects>
  $<INSTALL_INTERFACE:./lib/types>
)

target_link_libraries(generic fake-fast-led)

set(BUILD_SIMULATOR true CACHE BOOL "Whether to build the simulator")
if(BUILD_SIMULATOR)
  add_subdirectory(lib/simulator)
endif(BUILD_SIMULATOR)

enable_testing()

# Shared non-Test sources (FakeNetwork, VectorGenCommon) land both here and,
# via the glob below, directly in smalltests. That duplication is benign only
# because the linker prefers smalltests' own objects and never extracts the
# testlib archive copies - do not convert testlib to SHARED or add
# --whole-archive without deduplicating.
file(GLOB TEST_LIB "test/*.cpp" "test/*.hpp")
list(FILTER TEST_LIB EXCLUDE REGEX ".*Test\\.[ch]pp$")
list(FILTER TEST_LIB EXCLUDE REGEX ".*VectorGen\\.cpp$")
add_library(testlib ${TEST_LIB})
target_link_libraries(testlib generic fake-fast-led)

file(GLOB SMALLTEST_SOURCES "test/*.cpp" "test/*.hpp")
list(FILTER SMALLTEST_SOURCES EXCLUDE REGEX ".*InvalidPacketTest\\.[ch]pp$")
list(FILTER SMALLTEST_SOURCES EXCLUDE REGEX ".*VectorGen\\.cpp$")
add_executable(smalltests ${SMALLTEST_SOURCES})
target_link_libraries(smalltests generic gmock_main gtest fake-fast-led testlib
                      nlohmann_json::nlohmann_json)
# Lets ReferenceVectorTest locate the committed corpus from any build dir.
target_compile_definitions(smalltests PRIVATE
  REFERENCE_VECTORS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/sim/test/vectors/reference.json")
add_test(smalltests smalltests)

add_executable(largetests "test/InvalidPacketTest.cpp")
target_link_libraries(largetests generic gmock_main gtest fake-fast-led testlib)
add_test(largetests largetests)

# Renders firmware effects through the host-test fakes and prints reference
# vectors as JSON to stdout - the fidelity ground truth for the JS simulator
# port. Not part of smalltests/largetests; see
# specs/001-web-simulator/contracts/reference-vectors.md.
add_executable(vectorgen "test/VectorGen.cpp" "test/VectorGenCommon.cpp")
target_link_libraries(vectorgen generic fake-fast-led)
