#!/bin/bash
# Mode 2 -- the LAN multiplayer client, as shipped by its maintainers.
#
# cMod is consumed, not owned: we hold no copy of its source and no patch against it, so its next
# release is ours for free. This script only launches it and points it at the game data.
#
# cMod's "find the GOG installation automatically" is a Windows-path feature and does not fire on
# Linux, so the data directory is passed explicitly. fs_basepath must be a directory whose child is
# spelled exactly `baseEF` -- ours is a symlink to the installation, which keeps the install
# read-only and survives its path changing.
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CMOD="${CMOD_DIR:-$ROOT/build/cmod}"
HOME_DIR="${CMOD_HOME:-$ROOT/build/cmodhome}"

if [ "${1:-}" = "--dedicated" ]; then
    BIN="$CMOD/cMod-dedicated"; shift
    [ -x "$BIN" ] || { echo "cMod dedicated server not staged at $BIN"; exit 1; }
    mkdir -p "$HOME_DIR"
    exec "$BIN" +set fs_basepath "$ROOT/build" +set fs_homepath "$HOME_DIR" +set dedicated 1 "$@"
fi

BIN="$CMOD/cMod-stvoyHM"
[ -x "$BIN" ] || { echo "cMod client not staged at $BIN"; echo "staging: see docs/evidence/g7-cmod-staged.md"; exit 1; }
mkdir -p "$HOME_DIR"
exec "$BIN" +set fs_basepath "$ROOT/build" +set fs_homepath "$HOME_DIR" "$@"
