#!/usr/bin/env bash
# Run the native engine against the linked game data.
#
#   scripts/run-engine.sh [--build-dir DIR] [--engine PATH] [-- <extra engine args>]
#
# Three things this states explicitly, each because the engine will not guess them:
#   * where the data is      -- fs_basepath; the engine resolves it from the binary's own
#                               location, not the working directory
#   * where to WRITE          -- fs_homepath, deliberately NOT the data directory: baseEF is a
#                               symlink to the retail installation, and configuration and saves
#                               written through it would land inside that install
#   * where the libraries are -- dependencies are obtained rootlessly, so they are not on the
#                               system path

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build"
ENGINE=""
HOME_DIR=""
while [ $# -gt 0 ]; do
  case "$1" in
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --engine)    ENGINE="$2"; shift 2 ;;
    --home-dir)  HOME_DIR="$2"; shift 2 ;;
    --)          shift; break ;;
    *)           break ;;
  esac
done

[ -n "$ENGINE" ] || ENGINE="$(find "$ROOT" -maxdepth 3 -name longwayhome -type f 2>/dev/null | head -1)"
[ -n "$ENGINE" ] || { echo "engine binary not found; build it first (see engine/CMakeLists.txt, or run scripts/playtest-host-setup.sh)" >&2; exit 1; }
[ -d "$BUILD_DIR/baseEF" ] || { echo "no baseEF in $BUILD_DIR -- run scripts/playtest-host-setup.sh" >&2; exit 1; }

# Library search path, in the order the loader will use it:
#   1. the SP game modules -- the bridge dlopen()s "libefgame.so" and "libefui.so" by BARE NAME,
#      and the loader does not search the engine binary's own directory for those. Without this
#      the first map load fails with "dlopen libefgame.so failed" and looks like a crash.
#   2. the rootless dependency tree (durable copy first, then the ephemeral one if it survives).
for candidate in \
    "$ROOT/../upstream/efgame/build-linux" \
    "$ROOT/build-linux" \
    "$ROOT/../upstream/efgame/build" \
    "$BUILD_DIR/modules" \
    "$BUILD_DIR/deps/usr/lib/x86_64-linux-gnu" \
    "/tmp/sysroot/usr/lib/x86_64-linux-gnu"; do
  [ -d "$candidate" ] && export LD_LIBRARY_PATH="$candidate${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
done

# Say out loud whether the modules are where the loader will look, before the engine tries.
for m in libefgame.so libefui.so; do
  found=""
  for candidate in ${LD_LIBRARY_PATH//:/ }; do
    [ -f "$candidate/$m" ] && { found="$candidate/$m"; break; }
  done
  if [ -n "$found" ]; then echo "module:    $found"
  else echo "module:    $m NOT FOUND on the library path -- a map load will fail" >&2; fi
done

[ -n "$HOME_DIR" ] || HOME_DIR="$BUILD_DIR/home"
mkdir -p "$HOME_DIR"

LOG="$BUILD_DIR/run-$(date +%Y%m%d-%H%M%S).log"
echo "engine:    $ENGINE"
echo "data:      $BUILD_DIR/baseEF        (read-only; a symlink to the installation)"
echo "writes to: $HOME_DIR"
echo "log:       $LOG"
echo

# Tee rather than exec: what the engine prints is the evidence for whether a session worked,
# and a playtest that leaves no trace tells us nothing afterwards.
set +e
"$ENGINE" +set fs_basepath "$BUILD_DIR" +set fs_homepath "$HOME_DIR" "$@" 2>&1 | tee "$LOG"
echo
echo "exit: ${PIPESTATUS[0]}"
