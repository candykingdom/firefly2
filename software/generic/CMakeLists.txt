cmake_minimum_required(VERSION 2.8)
if (POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif (POLICY CMP0048)

project(firefly2 LANGUAGES CXX)

# Download GoogleTest for mocks and test runner.
include(DownloadProject.cmake)
download_project(PROJ                googletest
                 GIT_REPOSITORY      https://github.com/google/googletest.git
                 GIT_TAG             main
                 UPDATE_DISCONNECTED 1
)

include(DownloadProject.cmake)
download_project(PROJ                fake-fast-led
                 GIT_REPOSITORY      https://github.com/ademuri/FakeFastLED.git
                 GIT_TAG             master
                 UPDATE_DISCONNECTED 1
)

add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
add_subdirectory(${fake-fast-led_SOURCE_DIR} ${fake-fast-led_BINARY_DIR})

# Set appropriate warning levels and include debug symbols.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11 -g")

# Generate the file needed for YCM integration
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

file(GLOB LIBRARY_SOURCES
    "*.cpp"
    "FakeFastLED/*.cpp"
    "../../lib/color/*.cpp"
    "../../lib/debug/*.cpp"
    "../../lib/device/*.cpp"
    "../../lib/effect/*.cpp"
    "../../lib/led_manager/*.cpp"
    "../../lib/math/*.cpp"
    "../../lib/network/*.cpp"
    "../../lib/radio/*.cpp"
    "../../lib/types/*.cpp"
)
add_library(generic ${LIBRARY_SOURCES})
target_include_directories(generic PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/FakeFastLED>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../lib/types>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../lib/debug>
  $<INSTALL_INTERFACE:.>
  $<INSTALL_INTERFACE:./FakeFastLED>
  $<INSTALL_INTERFACE:../../lib/types>
  $<INSTALL_INTERFACE:../../lib/debug>)

target_link_libraries(generic fake-fast-led)

enable_testing()

file(GLOB TEST_SOURCES "test/*.cpp" "test/*.hpp")

add_executable(generictest ${TEST_SOURCES})

target_link_libraries(generictest generic gmock_main gtest fake-fast-led)
add_test(generictest generictest)
