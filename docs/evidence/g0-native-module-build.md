# G0 evidence — native x86_64 build of the single-player game module

Date: 2026-10-04. Gate: **G0, item 1** (module builds natively for x86_64 Linux).
Result: **PASS** — both modules build, link, and export exactly the intended entry points.

This proves the *build* gate. It does not yet prove behaviour: nothing has loaded these
libraries into an engine. That is G0's later items (script compiler) and G1's playtest.

## Environment

- Host: Debian/Ubuntu-family Linux, x86_64, 4 cores
- Toolchain: gcc/g++ 15.2.0, cmake 4.4.4, ninja 1.13.2 (cmake and ninja installed
  rootlessly into a venv — no system packages required)
- Upstream: `imjustadudegamer/VoyagerSP-Android` @ `0d8942e86a8469859da086f590875bcb66b4f4df`
- Build: `cmake -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release`, `-j4`
- Result: 157/157 compilation units, exit 0, zero errors

## Artifacts

| Artifact | Size (bytes) | SHA-256 |
|---|---|---|
| `libefgame.so` | 1,497,760 | `8b3b0135f4761fa0fa2edd36b0e1385dc371310b4bfa03fc79467158975a9a5e` |
| `libefui.so` | 582,832 | `c8365612fe6a82c0af90ca0540c62e854733dc519954a098fa4876e58b642be7` |

## Contract verification

- `libefgame.so` exports **only** `GetGameAPI`, `vmMain`, `dllEntry` (version script `EFGAME_1.0`).
  NEEDED: `libstdc++`, `libm`, `libgcc_s`, `libc`.
- `libefui.so` exports **only** `GetUIAPI` (version script `EFUI_1.0`).
  NEEDED: `libm`, `libc`.
- 92 undefined symbols, all libc/libstdc++ (`acosf`, `fopen`, `free`, `__cxa_*`, …) — i.e. the
  module reaches the engine exclusively through the passed-in function-pointer tables, as designed.
  Verified with `nm -D --defined-only` and `readelf -d`.

## Port friction fixed (the entire patch series, 37 files)

1. **clang-only flags.** `-ferror-limit=0` and `-Wno-everything` are clang options; native GCC
   rejects both. Now applied only when the compiler is clang, so the same CMakeLists serves Android
   and Linux.
2. **`std::find` without `<algorithm>`.** `speedrun/sound_skipping.cpp` relied on transitive
   includes. Added the include.
3. **Braced-init `std::filesystem::path{first, last}`.** Rejected by modern libstdc++. Made the
   conversion explicit via an intermediate `std::string`.
4. **`rand` macro poisoning libstdc++.** The Android compat shim did `#define rand() ef_rand15()`,
   which rewrites `using ::rand;` inside libstdc++'s `<stdlib.h>` into `using ::ef_rand15;` and breaks
   the TU. Fixed by making the macro clang-only and making the 107 call sites in 33 files explicit
   (`ef_rand15()`), preserving upstream's 15-bit rand semantics on GCC without poisoning any header.
5. **Brace-narrowing in UI tables** (`250 + MENU_BUTTON_MED_HEIGHT * 1.5` in an `int` array). MSVC and
   clang accept these silently; GCC calls them errors. Added `-Wno-narrowing` for GCC rather than
   editing Raven's tables, preserving original semantics.

## Reproduce

```
./scripts/bootstrap-upstream.sh
```

Clones the pinned upstream commit, applies `patches/`, and builds. No game data required to build.
