cmake_minimum_required(VERSION 2.8)
project(firefly2 LANGUAGES CXX)

# Set appropriate warning levels and include debug symbols.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -std=c++11 -g")

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_library(generic Radio.hpp Radio.cpp)
target_include_directories(generic PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:.>)

enable_testing()

file(GLOB TEST_SOURCES "test/*.cpp")

add_executable(generictest ${TEST_SOURCES})
target_link_libraries(generictest generic GTest::GTest GTest::Main)
add_test(generictest generictest)
