#!/usr/bin/env bash
# Prepare the playtest host: put the repository on the big volume and wire it to the game data.
#
#   scripts/playtest-host-setup.sh --repo-dir DIR --game-dir DIR [--repo-url URL]
#
#   --repo-dir   where to clone the repository (e.g. /big/long-way-home)
#   --game-dir   the existing Elite Force installation (e.g. /big/games/non-steam)
#
# The game data is linked, never copied and never committed: it is retail content under a
# non-commercial licence held by the owner, and this repository distributes none of it.
#
# Idempotent: safe to re-run. Existing clones are updated rather than re-created.

set -euo pipefail

REPO_DIR=""; GAME_DIR=""; REPO_URL="https://github.com/chezgoulet/long-way-home.git"
while [ $# -gt 0 ]; do
  case "$1" in
    --repo-dir) REPO_DIR="$2"; shift 2 ;;
    --game-dir) GAME_DIR="$2"; shift 2 ;;
    --repo-url) REPO_URL="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done
[ -n "$REPO_DIR" ] && [ -n "$GAME_DIR" ] || {
  echo "usage: playtest-host-setup.sh --repo-dir DIR --game-dir DIR" >&2; exit 2; }

echo "==> repository at $REPO_DIR"
if [ -d "$REPO_DIR/.git" ]; then
  git -C "$REPO_DIR" fetch --quiet origin
  git -C "$REPO_DIR" checkout --quiet testing
  git -C "$REPO_DIR" pull --quiet --ff-only origin testing
else
  mkdir -p "$(dirname "$REPO_DIR")"
  git clone --quiet --branch testing "$REPO_URL" "$REPO_DIR"
fi
git -C "$REPO_DIR" --no-pager log --oneline -1

echo "==> game data at $GAME_DIR"
# Missing game data is not a reason to refuse: the engine, the modules and the tools all
# build without it. The host just is not *playable* yet, and the script says so at the end.
PLAYABLE=no
if [ ! -d "$GAME_DIR" ]; then
  echo "    not found: $GAME_DIR"
  GAME_DIR=""
fi
# Case-insensitive: the GOG install ships "BaseEF", the engine looks for "baseEF", and on a
# case-sensitive filesystem those are different paths. List every spelling and match on name.
BASE="$(find "$GAME_DIR" -maxdepth 3 -type d -iname 'baseef' 2>/dev/null | head -1)"
if [ -z "$BASE" ]; then
  echo "    no baseEF under $GAME_DIR -- nothing to link yet (game not installed?)"
else
  PLAYABLE=yes
fi

if [ -n "$BASE" ]; then
PAKS=$(find "$BASE" -maxdepth 1 -iname 'pak*.pk3' | sort)
echo "    baseEF: $BASE"
echo "    paks:"
echo "$PAKS" | sed 's/^/      /'
if ! echo "$PAKS" | grep -qi 'pak0.pk3'; then
  echo "    WARNING: pak0.pk3 not found -- the engine will refuse to start"
  PLAYABLE=no
fi
fi
DLL=$(find "$GAME_DIR" -maxdepth 2 -iname 'efgamex86.dll' | head -1 || true)
echo "    efgamex86.dll (ownership check, never loaded): ${DLL:-NOT FOUND}"

# The engine searches ./baseEF relative to its working directory, so link rather than copy.
if [ -n "$BASE" ]; then
  # The engine searches ./baseEF relative to its working directory, so link rather than copy.
  LINK="$REPO_DIR/build/baseEF"
  mkdir -p "$(dirname "$LINK")"
  if [ -e "$LINK" ] && [ ! -L "$LINK" ]; then
    echo "    $LINK exists and is not a symlink; leaving it alone"
  else
    ln -sfn "$BASE" "$LINK"
    echo "    linked $LINK -> $BASE"
  fi
fi

echo
echo "PLAYTEST READY: $PLAYABLE"
[ "$PLAYABLE" = yes ] || echo "  (game data missing: the build will work, playing will not)"
echo "==> next: cd $REPO_DIR && ./scripts/bootstrap-upstream.sh"
