cmake_minimum_required(VERSION 3.16)
project(ibize CXX C)

# Native Linux replacement for Raven's IBIze.exe.
#
# Builds against a checked-out (and patched) upstream source tree:
#   cmake -S tools/ibize -B <build> -G Ninja -DUPSTREAM_DIR=/path/to/upstream
#
# The lexer/parser/writer are Raven's released ICARUS sources; only the
# command-line driver (ibize.cpp) is ours. The Android compatibility shim is
# force-included because the ICARUS tokenizer uses Win32 file APIs.

if(NOT UPSTREAM_DIR)
  message(FATAL_ERROR "Set -DUPSTREAM_DIR=<path to the upstream checkout>")
endif()

set(ICARUS ${UPSTREAM_DIR}/efgame/src/icarus)
set(COMPAT ${UPSTREAM_DIR}/efgame/android)

add_executable(ibize
  ibize.cpp
  ${ICARUS}/Tokenizer.cpp
  ${ICARUS}/Interpreter.cpp
  ${ICARUS}/BlockStream.cpp
)

target_include_directories(ibize PRIVATE
  ${COMPAT}          # Win32 stand-ins, must come first
  ${ICARUS}
)

set_target_properties(ibize PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

target_compile_options(ibize PRIVATE
  -fno-strict-aliasing
  -fcommon
  -include ${COMPAT}/ef_android_compat.h
  -Wno-narrowing
)
