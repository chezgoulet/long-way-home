# G1 evidence — the single-player engine builds natively on Linux

Date: 2026-10-04. Gate: **G1, milestone M2 (foundation)**. The engine that hosts our modules now
builds for desktop Linux — the first time this codebase has been built for anything but Android.

Artifact: `longwayhome`, 3,673,080 bytes, 192/192 translation units, exit 0.

## What it is

`engine/CMakeLists.txt` mirrors upstream's Android target source for source — the same explicit
client, qcommon, server, renderer and libmad lists — with the Android platform layer replaced:

- SDL2 and FFmpeg found directly rather than through pkg-config (the machines this builds on do not
  reliably have pkg-config, and a build that needs it is a build that fails where it matters).
- Desktop Vulkan instead of GLES3/EGL.
- `engine/platform/android/log.h` — `__android_log_print` and friends on stderr, same signatures.
- `engine/platform/sp_desktop_stubs.c` — desktop behaviour for symbols the bridge expects but that
  Android defines inside `#ifdef __ANDROID__` blocks.

## Nine things that had to be fixed, in order

Each was found by building, none by reading:

1. **SDL2 headers are split across the multiarch directory** on Debian-derived systems;
   `<SDL2/SDL_config.h>` includes `_real_SDL_config.h` from there.
2. **CMake found `libSDL2.a`, not `libSDL2.so`** — picking the static library drags in every private
   dependency SDL was built against (Wayland, DRM, X11 extensions) and fails to link on a desktop
   that has the shared library sitting right there. This was the cause of the earlier lilium link
   errors too, misread at the time as a Makefile quirk.
3. **The rootless dev tree's `libvulkan.so` symlink dangled** (no runtime beside it). The loader is
   found as a *file*, preferring a real one, and a dangling symlink is worse than nothing.
4. **`android/log.h` missing** — the platform layer, replaced.
5. **GCC 15 promotes incompatible-pointer-types and int-conversion to errors** where clang (which
   built the Android port) only warned. Kept visible, not fatal; implicit function declarations stay
   fatal, because that is what hid a real renderer bug upstream.
6. **Globbing directories pulled in files upstream's list omits** — `server/sv_rankings.c` drags in a
   Windows-only header (`..\rankings\N.N\gr\grapi.h`). The explicit list is the one that builds.
7. **`dladdr`/`Dl_info` are hidden by glibc without `_GNU_SOURCE`**, which bionic exposes
   unconditionally.
8. **`IN_ClearCrouchToggle` exists only inside `#ifdef __ANDROID__`** in `sdl_input.c` — it releases a
   sticky crouch-toggle held by the on-screen touch layout. A desktop has no touch controls, so a
   no-op is the correct implementation rather than a stub for missing work.
9. **`qftolsse`/`qvmftolsse`** (x86 float-to-int helpers) live in `asm/ftola.c`, which upstream never
   compiles because it targets ARM.

## What this does not prove

It has not been run yet. The binary needs SDL2's runtime, a display and the game data, all of which
are on the playtest host rather than the build host. Running it — and then getting it to load
`libefgame.so` and `libefui.so` instead of the retail `efgamex86.dll` — is the rest of M2.
