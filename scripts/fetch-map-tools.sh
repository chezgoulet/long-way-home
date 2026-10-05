#!/usr/bin/env bash
# Fetch the native map toolchain: q3map2 (the compiler) and mbspc, from a pinned release of
# NetRadiant-custom, into build/tools. Nothing is installed system-wide.
#
#   scripts/fetch-map-tools.sh
#
# The release is an AppImage inside a .7z. It is unpacked rather than run, so neither FUSE nor a
# display is needed; the 7z extractor is a pinned Python package run through uv.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLS="$ROOT/build/tools"
RELEASE="20260114"
ARCHIVE="netradiant-custom-$RELEASE-linux-x86_64.7z"
URL="https://github.com/Garux/netradiant-custom/releases/download/$RELEASE/$ARCHIVE"
SHA256="f48f6f1d0db2b910ef9cb5dc5d8a722852510f3c5c278dc17615c0466b8a7a3d"
DEST="$TOOLS/netradiant-custom-$RELEASE"

if [ -x "$DEST/usr/bin/q3map2" ]; then
  echo "map tools already present: $DEST/usr/bin"
  exit 0
fi
command -v uv >/dev/null 2>&1 || { echo "uv not found: it runs the pinned 7z extractor (https://docs.astral.sh/uv/)" >&2; exit 1; }

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT
mkdir -p "$TOOLS"

echo "==> downloading $ARCHIVE"
curl -fsSL -o "$WORK/$ARCHIVE" "$URL"
echo "$SHA256  $WORK/$ARCHIVE" | sha256sum --check --quiet || { echo "checksum mismatch: refusing to unpack" >&2; exit 1; }

echo "==> unpacking"
uv run --quiet --no-project --with 'py7zr==1.0.0' python -m py7zr x "$WORK/$ARCHIVE" "$WORK/x" >/dev/null
chmod +x "$WORK/x/NetRadiant-Custom-x86_64.AppImage"
(cd "$WORK/x" && ./NetRadiant-Custom-x86_64.AppImage --appimage-extract >/dev/null)
mv "$WORK/x/squashfs-root" "$DEST"

"$DEST/usr/bin/q3map2" 2>&1 | grep -m1 'Q3Map (ydnar)' || { echo "q3map2 did not run" >&2; exit 1; }
echo "map tools: $DEST/usr/bin (q3map2, mbspc)"
