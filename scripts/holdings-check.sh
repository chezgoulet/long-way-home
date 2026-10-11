#!/usr/bin/env bash
# The C2 ruling's guard: holdings is a fold over the marks, so no code may write or carry a stored
# holdings field (docs/morale.md, "`holdings` is a fold, not a stored value"; docs/gates.md, C2).
#
#   scripts/holdings-check.sh
#
# The scan is tools/ship/check_holdings.py. The guard is exercised in BOTH directions, as the brief
# requires: it must pass on the folded tree, and it must fail on a plant that reintroduces the field
# -- a guard that cannot fail for the reason we care about is decoration.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "==> holdings: no stored field in module/ship (the fold, not a float)"
python3 tools/ship/check_holdings.py module/ship

# The other direction: plant a stored holding and require the scan to catch it.
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT
cp -r module/ship "$WORK/ship"
cat >> "$WORK/ship/ship_core.cpp" <<'EOF'

// A plant for scripts/holdings-check.sh: a stored field must never pass the scan.
namespace ship { static void Planted(Ship &s) { s.crew[0].holdings = 0.5f; } }
EOF

if python3 tools/ship/check_holdings.py "$WORK/ship" >"$WORK/caught.txt" 2>&1; then
  echo "FAIL  the guard passed a plant that stores a holding: $WORK" >&2
  exit 1
fi
echo "    the plant is caught:"
sed 's/^/      /' "$WORK/caught.txt"

echo
echo "PASS  holdings is a fold; no stored field exists, and the guard catches one that is planted"
