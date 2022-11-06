set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -g")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS_DEBUG}")

FetchContent_Declare(fast-led-simulator
    GIT_REPOSITORY      https://github.com/ademuri/fast-led-simulator.git
    GIT_TAG             master
)
FetchContent_MakeAvailable(fast-led-simulator)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(Simulator Simulator.cpp)
target_link_libraries(Simulator fast-led-simulator generic)
