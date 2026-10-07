cmake_minimum_required(VERSION 3.16)
project(crew_core_tests CXX)

# Unit tests for the engine-independent half of the crew direction layer. Needs neither the
# upstream checkout nor any game data, so it runs anywhere a compiler does.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CREW ${CMAKE_CURRENT_SOURCE_DIR}/../../module/crew)
add_executable(test_crew_core test_crew_core.cpp ${CREW}/crew_core.cpp)
target_include_directories(test_crew_core PRIVATE ${CREW})
target_compile_options(test_crew_core PRIVATE -Wall -Wextra -Wpedantic -Werror)

enable_testing()
add_test(NAME crew_core COMMAND test_crew_core)
