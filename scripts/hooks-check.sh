#!/usr/bin/env bash
# The hook register's freshness check (docs/hook-register.md, Task D).
#
# Re-derives the public surface of module/ship/ship_core.h and fails when a public function exists
# in the code and not in the register -- the one omission a hand-written catalogue always makes.
# It does not judge reachability (that is a person's call), only presence.
#
#   scripts/hooks-check.sh
#
# Demonstrated failing by adding a declaration to ship_core.h and passing after removing it; see
# docs/evidence/hook-register.md.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

python3 tools/hooks/surface.py module/ship/ship_core.h >/dev/null
python3 tools/hooks/check_register.py --header module/ship/ship_core.h --register docs/hook-register.md
