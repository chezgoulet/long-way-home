cmake_minimum_required(VERSION 3.16)
project(ship_core_tests CXX)

# Unit tests for the engine-independent ship simulation (gate S1). Needs neither the upstream
# checkout nor any game data.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SHIP ${CMAKE_CURRENT_SOURCE_DIR}/../../module/ship)
add_executable(test_ship_core test_ship_core.cpp ${SHIP}/ship_core.cpp)
target_include_directories(test_ship_core PRIVATE ${SHIP})
target_compile_options(test_ship_core PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME ship_core COMMAND test_ship_core)
