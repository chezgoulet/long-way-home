#!/usr/bin/env bash
# The g_shipTest harness's case numbers are one namespace shared by every lane that adds a headless
# demonstration. A duplicate is silent: the earlier block runs first and most of them quit the run,
# so the later feature is present, its check fails, and nothing points at the cause. It happened --
# deck 14's check was dead for a day after rising claimed the same number.
#
#   scripts/harness-check.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

echo "==> harness: the g_shipTest case numbers are distinct"
python3 tools/harness/check_cases.py
