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
[ -n "$ENGINE" ] || { echo "engine binary not found; build it first (see engine/README.md)" >&2; exit 1; }
[ -d "$BUILD_DIR/baseEF" ] || { echo "no baseEF in $BUILD_DIR -- run scripts/playtest-host-setup.sh" >&2; exit 1; }

# Library search path: the durable copy first, then the ephemeral rootless tree if it survives,
# then whatever the system already provides.
for candidate in "$BUILD_DIR/deps/usr/lib/x86_64-linux-gnu" "/tmp/sysroot/usr/lib/x86_64-linux-gnu"; do
  [ -d "$candidate" ] && export LD_LIBRARY_PATH="$candidate${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
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
