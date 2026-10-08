#!/usr/bin/env bash
# Long Way Home -- the one launcher (gate G1, docs/the-entry-point.md).
#
#   scripts/run-lwh.sh                 install the ship content and launch the ship
#   scripts/run-lwh.sh --menu          install it and stop at the main menu, to choose from there
#   scripts/run-lwh.sh -- <engine args>   extra arguments, passed through (after --)
#
# What it does, and why each part is here:
#
#   * installs the merged ship's pak and configs/lwh-start.cfg into the playtest home. The config is
#     the one place the run's values live; this script does not restate them, it executes them.
#   * sets com_hunkMegs 768. That is an engine memory allocation, not a run value: it is LATCHed and
#     consumed before the filesystem is up, so it cannot come from an exec'd config. The whole-ship
#     BSP is ~54 MB and the default 128 MB hunk fails with Hunk_AllocateTempMemory.
#   * without --menu, execs the config, which sets g_ship and g_shipDeckPitch and loads `map voyager`.
#     With --menu the engine comes up on the retail main menu and the "Long Way Home" line starts the
#     run through exactly the same config.
#
# It changes no cvar defaults: g_ship is still 0 in the code, so a game started any other way boots
# the retail campaign and Holomatch exactly as before.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"
HOME_DIR="$BUILD/home"
GAME_DIR="$HOME_DIR/baseEF"
PAK="$BUILD/ship/out/longway_voyager.pk3"
CFG="$ROOT/configs/lwh-start.cfg"
MENU=no

while [ $# -gt 0 ]; do
  case "$1" in
    --menu) MENU=yes; shift ;;
    --)     shift; break ;;
    *)      break ;;
  esac
done

[ -f "$CFG" ] || { echo "missing $CFG" >&2; exit 1; }
if [ ! -f "$PAK" ]; then
  echo "no merged ship at $PAK -- run scripts/build-ship.sh first" >&2
  exit 1
fi

# Install the content where the engine will find it, beside the retail paks in the writable home.
mkdir -p "$GAME_DIR"
cp -f "$PAK" "$GAME_DIR/"
cp -f "$CFG" "$GAME_DIR/"

# A killed run leaves "Lilium Voyager SP.pid" behind, and a stale one makes the next start hang in
# Com_Init (the single-instance check waits on a pid that no longer exists). The other check scripts
# clear them for the same reason; do it here so the launcher is re-runnable after a Ctrl-C.
find "$HOME_DIR" -maxdepth 2 -name '*.pid' -delete 2>/dev/null || true
echo "installed: $GAME_DIR/$(basename "$PAK")"
echo "installed: $GAME_DIR/$(basename "$CFG")"
echo

if [ "$MENU" = yes ]; then
  echo "starting at the main menu -- choose 'LONG WAY HOME'"
  exec "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" +set com_hunkMegs 768 "$@"
else
  exec "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" +set com_hunkMegs 768 \
      +exec lwh-start.cfg "$@"
fi
