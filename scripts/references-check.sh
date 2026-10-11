#!/usr/bin/env bash
# The script references the published map sources carry, against the GDK corpus they must resolve
# in -- and the boundary no corpus satisfies.
#
#   scripts/references-check.sh [--map-root DIR] [--corpus DIR] [--json OUT]
#
# Defaults: build/gdk/maps and build/gdk/scripts, both restored by scripts/fetch-gdk.sh.
#
# This is the guard for the residual "the GDK corpus lacks N script references": it re-runs the
# validator over every published map source and fails when the unresolved set is not exactly the
# recorded boundary (two malformed `usescript` values in shipped maps). It also names the
# references that are present in the corpus but missed by a case-sensitive `*.txt` glob, which is
# what made the old count far larger than the corpus warranted.
#
#   tools/validator/reference-census.py   the census itself
#   docs/evidence/the-126-references.md   the finding, the classes and the reasons

set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

MAP_ROOT="$ROOT/build/gdk/maps"
CORPUS="$ROOT/build/gdk/scripts"
JSON=""
while [ $# -gt 0 ]; do
  case "$1" in
    --map-root) MAP_ROOT="$2"; shift 2 ;;
    --corpus)   CORPUS="$2"; shift 2 ;;
    --json)     JSON="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

[ -d "$MAP_ROOT" ] || { echo "map sources not found: $MAP_ROOT -- run scripts/fetch-gdk.sh" >&2; exit 2; }
[ -d "$CORPUS" ]   || { echo "script corpus not found: $CORPUS -- run scripts/fetch-gdk.sh" >&2; exit 2; }

exec python3 "$ROOT/tools/validator/reference-census.py" \
  --map-root "$MAP_ROOT" --corpus "$CORPUS" ${JSON:+--json "$JSON"}
