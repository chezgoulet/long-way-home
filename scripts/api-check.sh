#!/usr/bin/env bash
# The ship's API: the enumerated command surface the computer may answer from (docs/evidence/
# the-computer-api.md, M6 Task A).
#
# The enumeration lives in module/ship/console_api.def and is read by the module at compile time
# (ship::ConsoleVerbCount/ConsoleVerbAt/FindConsoleVerb). This check derives the surface the console
# `Svcmd_Ship_f` actually answers from module/ship/g_ship.cpp and fails when the two drift in either
# direction -- a console verb missing from the API, or an API verb the console does not answer. It is
# the hooks-check.sh pattern (docs/hook-register.md) applied to the console.
#
#   scripts/api-check.sh
#
# Demonstrated failing in both directions, and passing on the tree, in docs/evidence/the-computer-api.md.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

python3 tools/console/verbs.py check \
  --console module/ship/g_ship.cpp \
  --register module/ship/console_api.def
