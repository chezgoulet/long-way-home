cmake_minimum_required(VERSION 3.16)
project(longwayhome_engine C CXX)

# Native Linux build of the single-player engine: the port's engine tree (lilium-voyager
# lineage + the Vulkan renderer) plus the SP bridge that hosts libefgame.so/libefui.so.
#
# Mirrors upstream's Android target (EFAndroid-SP/app/jni/src/CMakeLists.txt) source for
# source, with the Android platform layer replaced:
#   * SDL2 from the system/rootless tree instead of the vendored Android copy
#   * desktop Vulkan instead of GLES3/EGL
#   * the Android log shim replaced by stderr (see engine/platform/)
#   * FFmpeg for Bink cutscenes linked dynamically instead of the ARM static libs
#
#   cmake -S engine -B <build> -DUPSTREAM_DIR=/path/to/upstream

if(NOT UPSTREAM_DIR)
  message(FATAL_ERROR "Set -DUPSTREAM_DIR=<path to the upstream checkout>")
endif()

set(EF ${UPSTREAM_DIR}/EFAndroid-SP/app/jni/efcode)
set(SP ${UPSTREAM_DIR}/EFAndroid-SP/app/jni/sp)
set(EFG ${UPSTREAM_DIR}/efgame)
if(NOT EXISTS ${EF}/client/cl_main.c)
  message(FATAL_ERROR "UPSTREAM_DIR does not look like the patched upstream checkout: ${UPSTREAM_DIR}")
endif()

set(LONGWAY_SYSROOT "" CACHE PATH "Optional prefix holding dependency headers and libraries")
set(_lib_hints ${LONGWAY_SYSROOT}/usr/lib/x86_64-linux-gnu ${LONGWAY_SYSROOT}/usr/lib)
set(_inc_hints ${LONGWAY_SYSROOT}/usr/include ${LONGWAY_SYSROOT}/usr/include/x86_64-linux-gnu)

# Prefer shared libraries: picking up a static libSDL2.a drags in every private dependency SDL
# was built against (Wayland, DRM, X11) and fails to link where the shared library exists.
set(CMAKE_FIND_LIBRARY_SUFFIXES ".so;.a")

option(LONGWAY_BINK "Link FFmpeg for Bink cutscene playback" ON)

# ---- engine source groups (same lists as the Android target) ----
# Explicit lists, mirroring upstream's Android target. Globbing the directories instead pulls in
# files their build deliberately omits -- server/sv_rankings.c drags in a Windows-only header
# (`..\rankings\N.N\gr\grapi.h`), and qcommon/*.c would pick up the Windows VM JIT. The
# explicit list is the one that is known to build.
set(CLIENT_SRC
  ${EF}/client/cl_cgame.c ${EF}/client/cl_cin.c ${EF}/client/cl_console.c
  ${EF}/client/cl_input.c ${EF}/client/cl_keys.c ${EF}/client/cl_main.c
  ${EF}/client/cl_net_chan.c ${EF}/client/cl_parse.c ${EF}/client/cl_scrn.c
  ${EF}/client/cl_ui.c ${EF}/client/cl_avi.c
  ${EF}/client/snd_adpcm.c ${EF}/client/snd_dma.c ${EF}/client/snd_mem.c
  ${EF}/client/snd_mix.c ${EF}/client/snd_wavelet.c ${EF}/client/snd_main.c
  ${EF}/client/snd_codec.c ${EF}/client/snd_codec_wav.c ${EF}/client/snd_codec_mp3.c
  ${EF}/client/qal.c ${EF}/client/snd_openal.c)

set(QCOMMON_SRC
  ${EF}/qcommon/cm_load.c ${EF}/qcommon/cm_patch.c ${EF}/qcommon/cm_polylib.c
  ${EF}/qcommon/cm_test.c ${EF}/qcommon/cm_trace.c ${EF}/qcommon/cmd.c
  ${EF}/qcommon/common.c ${EF}/qcommon/cvar.c ${EF}/qcommon/files.c
  ${EF}/qcommon/md4.c ${EF}/qcommon/md5.c ${EF}/qcommon/msg.c
  ${EF}/qcommon/net_chan.c ${EF}/qcommon/net_ip.c ${EF}/qcommon/huffman.c
  ${EF}/qcommon/q_math.c ${EF}/qcommon/q_shared.c ${EF}/qcommon/unzip.c
  ${EF}/qcommon/ioapi.c ${EF}/qcommon/puff.c ${EF}/qcommon/vm.c
  ${EF}/qcommon/vm_interpreted.c
  # x86 float-to-int helpers (qftolsse/qvmftolsse). ioquake3 compiles this on x86; upstream's
  # Android target never needs it, so it is absent from their list. Still required here.
  ${EF}/asm/ftola.c)

set(SERVER_SRC
  ${EF}/server/sv_bot.c ${EF}/server/sv_ccmds.c ${EF}/server/sv_client.c
  ${EF}/server/sv_game.c ${EF}/server/sv_init.c ${EF}/server/sv_main.c
  ${EF}/server/sv_net_chan.c ${EF}/server/sv_snapshot.c ${EF}/server/sv_world.c)
file(GLOB BOTLIB_SRC  ${EF}/botlib/*.c)
file(GLOB SDL_SRC     ${EF}/sdl/*.c)
file(GLOB RENDERCOMMON_SRC ${EF}/renderercommon/*.c)
file(GLOB RENDERER_SRC     ${EF}/renderervk/*.c)
file(GLOB ZLIB_SRC ${EF}/zlib/*.c)
file(GLOB JPEG_SRC ${EF}/jpeg-8c/*.c)
file(GLOB LIBMAD_SRC ${EF}/libmad-0.15.1b/*.c)

# x86_64 has no compiled VM in this tree: run the bytecode interpreter (as Android does
# for any ABI without a JIT).
set(EF_VM_JIT FALSE)

# upstream excludes these two from the renderer build; keep that
list(REMOVE_ITEM RENDERER_SRC ${EF}/renderervk/tr_subs.c)
list(REMOVE_ITEM RENDERER_SRC ${EF}/renderervk/shaders/spirv/shader_data.c)

set(SYS_SRC ${EF}/sys/con_log.c ${EF}/sys/con_passive.c ${EF}/sys/sys_autoupdater.c ${EF}/sys/sys_unix.c)
set(SYS_MAIN ${EF}/sys/sys_main.c)

# ---- SP bridge (game + ui modules hosted through dlopen) ----
add_library(sp_game_bridge STATIC
  ${SP}/sp_bridge.cpp ${SP}/sp_ui_bridge.cpp ${SP}/sp_clink.c)
target_include_directories(sp_game_bridge PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/platform
  ${_inc_hints}
  ${EFG}/android ${EFG}/src/game ${EFG}/src/cgame ${EFG}/src/ui
  ${EFG}/src/client ${EFG}/src/renderer ${EFG}/src/qcommon ${SP})
target_compile_options(sp_game_bridge PRIVATE
  -include ${EFG}/android/ef_android_compat.h -fno-threadsafe-statics -fcommon -w)
target_compile_definitions(sp_game_bridge PRIVATE NDEBUG)

# ---- the engine binary ----
set(ENGINE_SRC
  ${CLIENT_SRC} ${QCOMMON_SRC} ${SERVER_SRC} ${BOTLIB_SRC}
  ${SDL_SRC} ${SYS_SRC} ${SYS_MAIN}
  ${RENDERER_SRC} ${RENDERCOMMON_SRC}
  ${ZLIB_SRC} ${JPEG_SRC} ${LIBMAD_SRC}
  ${SP}/sp_integration.c
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/sp_desktop_stubs.c)

# Dependencies are found directly rather than through pkg-config: the machines this has to
# build on (the playtest host, a rootless toolchain tree) do not reliably have pkg-config or
# the .pc files, and a build that needs them is a build that fails where it matters.

find_path(SDL2_INCLUDE_DIR NAMES SDL.h PATH_SUFFIXES SDL2 HINTS ${_inc_hints})
find_library(SDL2_LIBRARY NAMES SDL2 HINTS ${_lib_hints})

find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h HINTS ${_inc_hints})
# The loader is found as a FILE, not via find_library: a rootless dev tree ships the
# libvulkan.so -> libvulkan.so.1 symlink with no runtime beside it, and a dangling symlink is
# worse than nothing here. Prefer a real loader wherever it lives.
find_file(VULKAN_RUNTIME NAMES libvulkan.so.1 libvulkan.so
          HINTS ${_lib_hints} /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib)
if(NOT VULKAN_INCLUDE_DIR OR NOT VULKAN_RUNTIME)
  message(FATAL_ERROR "Vulkan loader/headers not found. Pass -DLONGWAY_SYSROOT=<prefix>.")
endif()
message(STATUS "Vulkan: ${VULKAN_RUNTIME}")

set(LONGWAY_FFMPEG_LIBS "")
if(LONGWAY_BINK)
  find_path(AVCODEC_INCLUDE_DIR NAMES libavcodec/avcodec.h HINTS ${_inc_hints})
  find_library(AVCODEC_LIBRARY   NAMES avcodec   HINTS ${_lib_hints})
  find_library(AVFORMAT_LIBRARY  NAMES avformat  HINTS ${_lib_hints})
  find_library(AVUTIL_LIBRARY    NAMES avutil    HINTS ${_lib_hints})
  find_library(SWSCALE_LIBRARY   NAMES swscale   HINTS ${_lib_hints})
  if(AVCODEC_INCLUDE_DIR AND AVCODEC_LIBRARY AND AVFORMAT_LIBRARY AND AVUTIL_LIBRARY AND SWSCALE_LIBRARY)
    list(APPEND ENGINE_SRC ${SP}/bink_ff.c)
    list(APPEND LONGWAY_FFMPEG_LIBS ${AVCODEC_LIBRARY} ${AVFORMAT_LIBRARY}
         ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY})
    message(STATUS "Bink cutscene playback: enabled (FFmpeg found at ${AVCODEC_LIBRARY})")
  else()
    # Not fatal: cinematics are polish. A NOTFOUND variable reaching target_link_libraries is a
    # CMake *generate* error, which is a confusing way to learn that a package is missing.
    message(WARNING "FFmpeg not found: building without Bink cutscene playback")
    set(LONGWAY_BINK OFF)
  endif()
endif()

if(NOT LONGWAY_BINK)
  list(APPEND ENGINE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/platform/bink_stubs.c)
endif()

if(NOT SDL2_INCLUDE_DIR OR NOT SDL2_LIBRARY)
  message(FATAL_ERROR "SDL2 not found. Pass -DLONGWAY_SYSROOT=<prefix> if it is not installed system-wide.")
endif()

add_executable(longwayhome ${ENGINE_SRC})

target_include_directories(longwayhome PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/platform
  ${EF} ${EF}/zlib ${EF}/jpeg-8c ${EF}/AL
  ${EF}/libmad-0.15.1b ${EF}/libmad-0.15.1b/include
  ${SP} ${SDL2_INCLUDE_DIR} ${_inc_hints} ${VULKAN_INCLUDE_DIR}
  $<$<BOOL:${AVCODEC_INCLUDE_DIR}>:${AVCODEC_INCLUDE_DIR}>)

target_compile_definitions(longwayhome PRIVATE
  _GNU_SOURCE
  ELITEFORCE BOTLIB
  USE_CODEC_MP3 USE_INTERNAL_MP3 FPM_64BIT
  NO_VM_COMPILED
  USE_VULKAN USE_VULKAN_API
  USE_FLEXIBLE_DISPLAY=1
  USE_OPENAL USE_OPENAL_DLOPEN
  USE_LOCAL_HEADERS=1
  PRODUCT_VERSION="1.40"
  ARCH_STRING="x86_64"
  # Desktop default data root. The engine also honours fs_basepath; this only supplies a
  # fallback when neither is given.
  LONGWAY_DEFAULT_DATA_ROOT="/home/c/big/git/long-way-home/build")

target_compile_options(longwayhome PRIVATE
  -include ${CMAKE_CURRENT_SOURCE_DIR}/platform/desktop_compat.h
  -fno-strict-aliasing -fcommon
  # GCC 15 promotes several diagnostics to errors that the code was written under (and that
  # clang, which built the Android port, only warned about): incompatible pointer types on
  # const-qualification, and implicit int conversions. The code is 25 years old and correct in
  # practice; keep the diagnostics visible without failing the build on them. Implicit function
  # declarations stay fatal -- those hid a real corruption bug in the renderer (see upstream).
  -Wno-error=incompatible-pointer-types -Wno-incompatible-pointer-types
  -Wno-error=int-conversion -Wno-int-conversion
  -Werror=implicit-function-declaration
  -O2 -fomit-frame-pointer)

# SDL2's optional backends (Wayland/libdecor, and others) reference libraries that may not be
# installed wherever this is built. They are resolved by the dynamic loader at runtime if that
# backend is actually used, so the link must not insist on seeing them now.
target_link_options(longwayhome PRIVATE -Wl,--allow-shlib-undefined)

target_link_libraries(longwayhome
  sp_game_bridge
  ${SDL2_LIBRARY} ${VULKAN_RUNTIME} dl m pthread
  ${LONGWAY_FFMPEG_LIBS})
