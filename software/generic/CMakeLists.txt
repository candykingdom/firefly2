cmake_minimum_required(VERSION 2.8)
project(firefly2 LANGUAGES CXX)

# Download GoogleTest for mocks and test runner.
include(DownloadProject.cmake)
download_project(PROJ                googletest
                 GIT_REPOSITORY      https://github.com/google/googletest.git
                 GIT_TAG             master
                 UPDATE_DISCONNECTED 1
)

add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})

# Set appropriate warning levels and include debug symbols.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11 -g")

# Generate the file needed for YCM integration
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

file(GLOB LIBRARY_SOURCES "*.cpp" "FakeFastLED/*.cpp")
add_library(generic ${LIBRARY_SOURCES})
target_include_directories(generic PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/FakeFastLED>
  $<INSTALL_INTERFACE:.>
  $<INSTALL_INTERFACE:./FakeFastLED>)

enable_testing()

file(GLOB TEST_SOURCES "test/*.cpp" "test/*.hpp")

add_executable(generictest ${TEST_SOURCES})

target_link_libraries(generictest generic gmock_main gtest)
add_test(generictest generictest)
