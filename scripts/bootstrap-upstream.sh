#!/usr/bin/env bash
# Bootstrap and build the native Linux single-player game module.
#
#   ./scripts/bootstrap-upstream.sh [workdir]
#
# Clones the upstream reference port at a pinned commit into workdir (default
# ../upstream), applies this repository's patch series, and builds the two
# single-player modules for the native host.
#
# Requirements: git, cmake >= 3.16, ninja, a C/C++17 toolchain.
# On hosts without cmake/ninja, they can be obtained without root:
#   python3 -m venv .venv && .venv/bin/pip install cmake ninja
#
# No game data is needed to build. Only to run.

set -euo pipefail

# Pick a CMake generator that exists on this machine: ninja when present, otherwise the
# Makefiles generator. The playtest host has cmake and make but no ninja, and a build script
# that refuses to run there is a build script that does not work where it is needed.
if command -v ninja >/dev/null 2>&1; then
  GENERATOR="Ninja"
elif command -v make >/dev/null 2>&1; then
  GENERATOR="Unix Makefiles"
else
  echo "neither ninja nor make found: install one of them" >&2
  exit 1
fi

UPSTREAM_REPO="https://github.com/imjustadudegamer/VoyagerSP-Android.git"
UPSTREAM_SHA="0d8942e86a8469859da086f590875bcb66b4f4df"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Locate cmake/ninja. They are frequently installed rootlessly in a venv rather than system-wide;
# a bare `cmake: command not found` deep inside a build is a bad way to learn that.
if ! command -v cmake >/dev/null 2>&1 || ! command -v ninja >/dev/null 2>&1; then
  for candidate in "$HOME/.cache/lwh-venv/bin" "$ROOT/.venv/bin" "$ROOT/../.venv/bin" "/usr/local/bin"; do
    if [ -x "$candidate/cmake" ]; then PATH="$candidate:$PATH"; break; fi
  done
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake not found on PATH." >&2
  echo "  install it, or do this without root:" >&2
  echo "    python3 -m venv .venv && .venv/bin/pip install cmake ninja" >&2
  exit 1
fi
# ninja is preferred, not required: see the GENERATOR choice above.
WORK="${1:-$(dirname "$ROOT")/upstream}"
PATCHES="$ROOT/patches"

echo "==> upstream: $UPSTREAM_REPO @ $UPSTREAM_SHA"
if [ ! -d "$WORK/.git" ]; then
  git clone --quiet "$UPSTREAM_REPO" "$WORK"
fi
git -C "$WORK" fetch --quiet origin "$UPSTREAM_SHA" || true
git -C "$WORK" checkout --quiet "$UPSTREAM_SHA"
git -C "$WORK" reset --hard --quiet "$UPSTREAM_SHA"
git -C "$WORK" clean -qfd

echo "==> applying $(find "$PATCHES" -name '*.patch' | wc -l) patch(es)"
for p in "$PATCHES"/*.patch; do
  echo "    $(basename "$p")"
  git -C "$WORK" apply --whitespace=nowarn "$p"
done

echo "==> configuring"
# LWH_MODULE_DIR compiles this repository's own game logic (module/) into the game module; the
# hooks patch 0005 adds are empty inlines without it.
cmake -S "$WORK/efgame" -B "$WORK/efgame/build-linux" -G "$GENERATOR" -DCMAKE_BUILD_TYPE=Release \
  -DLWH_MODULE_DIR="$ROOT/module"

echo "==> building"
cmake --build "$WORK/efgame/build-linux" -j"$(nproc)"

echo "==> note: the entity dictionaries (SP_entities.def, HM_entities-def.txt) come from the"
echo "    Game Development Kit, which this repository does not vendor. Copy them into"
echo "    $WORK to enable the validator's entity-class check."

echo "==> artifacts (game modules)"
ls -la "$WORK/efgame/build-linux"/libefgame.so "$WORK/efgame/build-linux"/libefui.so

echo "==> building the native ICARUS script compiler (ibize)"
cmake -S "$ROOT/tools/ibize" -B "$WORK/ibize-build" -G "$GENERATOR" \
  -DUPSTREAM_DIR="$WORK" -DCMAKE_BUILD_TYPE=Release
cmake --build "$WORK/ibize-build" -j"$(nproc)"
ls -la "$WORK/ibize-build/ibize"

echo "==> building the round-trip reader (ibi-dump)"
cmake -S "$ROOT/tools/ibi-dump" -B "$WORK/ibi-dump-build" -G "$GENERATOR" \
  -DUPSTREAM_DIR="$WORK" -DCMAKE_BUILD_TYPE=Release
cmake --build "$WORK/ibi-dump-build" -j"$(nproc)"
ls -la "$WORK/ibi-dump-build/ibi-dump"
echo
# The module links with --unresolved-symbols=ignore-all (engine syscalls resolve at dlopen), so a
# missing symbol of our own would not fail the link. Check the exports instead of trusting it.
echo "==> checking exports"
exports="$(nm -D --defined-only "$WORK/efgame/build-linux/libefgame.so" | awk '$2 == "T" {print $3}' | sort | tr '\n' ' ')"
ui_exports="$(nm -D --defined-only "$WORK/efgame/build-linux/libefui.so" | awk '$2 == "T" {print $3}' | sort | tr '\n' ' ')"
echo "  libefgame.so : $exports"
echo "  libefui.so   : $ui_exports"
for want in GetGameAPI vmMain dllEntry; do
  case "$exports" in *"$want"*) ;; *) echo "libefgame.so does not export $want" >&2; exit 1 ;; esac
done
case "$ui_exports" in *GetUIAPI*) ;; *) echo "libefui.so does not export GetUIAPI" >&2; exit 1 ;; esac
[ "$(echo "$exports" | wc -w)" -eq 3 ] || { echo "libefgame.so exports more than its three entry points" >&2; exit 1; }
if nm -D --undefined-only "$WORK/efgame/build-linux/libefgame.so" | grep -q 'Crew_'; then
  echo "libefgame.so has unresolved Crew_ symbols: the direction layer was not compiled in" >&2; exit 1
fi
echo "  ok: the intended entry points and nothing else"
