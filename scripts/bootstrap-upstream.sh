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
if ! command -v ninja >/dev/null 2>&1; then
  echo "ninja not found on PATH (cmake can use make with -G 'Unix Makefiles', or install ninja)." >&2
  exit 1
fi

UPSTREAM_REPO="https://github.com/imjustadudegamer/VoyagerSP-Android.git"
UPSTREAM_SHA="0d8942e86a8469859da086f590875bcb66b4f4df"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
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

echo "==> applying $(ls "$PATCHES"/*.patch | wc -l) patch(es)"
for p in "$PATCHES"/*.patch; do
  echo "    $(basename "$p")"
  git -C "$WORK" apply --whitespace=nowarn "$p"
done

echo "==> configuring"
cmake -S "$WORK/efgame" -B "$WORK/efgame/build-linux" -G Ninja -DCMAKE_BUILD_TYPE=Release

echo "==> building"
cmake --build "$WORK/efgame/build-linux" -j"$(nproc)"

echo "==> note: the entity dictionaries (SP_entities.def, HM_entities-def.txt) come from the"
echo "    Game Development Kit, which this repository does not vendor. Copy them into"
echo "    $WORK to enable the validator's entity-class check."

echo "==> artifacts (game modules)"
ls -la "$WORK/efgame/build-linux"/libefgame.so "$WORK/efgame/build-linux"/libefui.so

echo "==> building the native ICARUS script compiler (ibize)"
cmake -S "$ROOT/tools/ibize" -B "$WORK/ibize-build" -G Ninja \
  -DUPSTREAM_DIR="$WORK" -DCMAKE_BUILD_TYPE=Release
cmake --build "$WORK/ibize-build" -j"$(nproc)"
ls -la "$WORK/ibize-build/ibize"

echo "==> building the round-trip reader (ibi-dump)"
cmake -S "$ROOT/tools/ibi-dump" -B "$WORK/ibi-dump-build" -G Ninja \
  -DUPSTREAM_DIR="$WORK" -DCMAKE_BUILD_TYPE=Release
cmake --build "$WORK/ibi-dump-build" -j"$(nproc)"
ls -la "$WORK/ibi-dump-build/ibi-dump"
echo
echo "Expected exports:"
echo "  libefgame.so : GetGameAPI, vmMain, dllEntry"
echo "  libefui.so   : GetUIAPI"
