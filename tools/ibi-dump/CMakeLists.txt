cmake_minimum_required(VERSION 3.16)
project(ibi_dump CXX C)

# Reads a compiled ICARUS block stream back using the game's own reader, so a compiled
# script can be verified without the game or the original compiler.
#   cmake -S tools/ibi-dump -B <build> -G Ninja -DUPSTREAM_DIR=/path/to/upstream

if(NOT UPSTREAM_DIR)
  message(FATAL_ERROR "Set -DUPSTREAM_DIR=<path to the upstream checkout>")
endif()

set(ICARUS ${UPSTREAM_DIR}/efgame/src/icarus)
set(COMPAT ${UPSTREAM_DIR}/efgame/android)

add_executable(ibi-dump
  ibi-dump.cpp
  ${ICARUS}/BlockStream.cpp
)

target_include_directories(ibi-dump PRIVATE ${COMPAT} ${ICARUS})
set_target_properties(ibi-dump PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_options(ibi-dump PRIVATE -fno-strict-aliasing -fcommon -Wno-narrowing
  -include ${COMPAT}/ef_android_compat.h)
