#!/usr/bin/env bash
# Run the native engine against the linked game data.
#
#   scripts/run-engine.sh [--build-dir DIR] [--engine PATH] [-- <extra engine args>]
#
# Wraps the two things the engine needs stated explicitly: where its data is (it resolves
# fs_basepath from the binary's own location, not the working directory) and, when the
# dependencies were obtained rootlessly, where their libraries live.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build"
ENGINE=""
SYSROOT="${LONGWAY_SYSROOT:-/tmp/sysroot}"

while [ $# -gt 0 ]; do
  case "$1" in
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --engine)    ENGINE="$2"; shift 2 ;;
    --)          shift; break ;;
    *)           break ;;
  esac
done

[ -n "$ENGINE" ] || ENGINE="$(find "$ROOT" "$BUILD_DIR" -maxdepth 3 -name longwayhome -type f 2>/dev/null | head -1)"
[ -n "$ENGINE" ] || { echo "engine binary not found; build it first (see engine/README.md)" >&2; exit 1; }
[ -d "$BUILD_DIR/baseEF" ] || { echo "no baseEF in $BUILD_DIR -- run scripts/playtest-host-setup.sh" >&2; exit 1; }

if [ -d "$SYSROOT/usr/lib/x86_64-linux-gnu" ]; then
  export LD_LIBRARY_PATH="$SYSROOT/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

LOG="$BUILD_DIR/run-$(date +%Y%m%d-%H%M%S).log"
echo "engine:    $ENGINE"
echo "data:      $BUILD_DIR/baseEF"
echo "log:       $LOG"
echo

# Tee rather than exec: what the engine prints is the evidence for whether a session
# worked, and a playtest that leaves no trace tells us nothing afterwards.
"$ENGINE" +set fs_basepath "$BUILD_DIR" +set fs_homepath "$BUILD_DIR" "$@" 2>&1 | tee "$LOG"
