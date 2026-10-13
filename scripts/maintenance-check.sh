#!/usr/bin/env bash
# Preventive maintenance is the seventh job kind (the owner's ruling 2026-10-12, register row 3.23,
# docs/crew-work.md). This is the demonstration and the falsification.
#
#   scripts/maintenance-check.sh
#
# Part one -- the demonstration. The unit test suite runs, and `test_ship_core --maintenance` prints
# the seventh kind as evidence (docs/evidence/the-maintenance-kind.md): the kind and the six it joins;
# the axis (a system's **wear**, the deferred-maintenance clock) and the threshold (MAINTENANCE_DUE)
# as numbers; the rule that connects the crew-hours to the odds; the same system maintained and not,
# over fifteen days, with the wear and the failure odds moving; the order and who may give it; and
# the save's cost (the wear clock is saved, so the format moved 63 -> 64 and an older save does not
# load). The lines asserted here are the acceptance, so a silent regression fails the check.
#
# Part two -- the falsification, the shape the brief names: a maintenance that runs but changes
# nothing. The work loop's wear reduction is planted out (`gain = 0.0f`), so the job is raised and
# worked but the clock does not move; the test must FAIL on the seventh kind, then restore. A guard
# that cannot fail for the reason we care about is decoration.
#
# Static host: no engine and no game data; the model is exercised directly, so this runs anywhere a
# compiler does.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

fail() { echo "FAIL  $1" >&2; exit 1; }

echo "==> maintenance: the unit test suite, built here"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> maintenance: the demonstration (test_ship_core --maintenance)"
"$WORK/test_ship_core" --maintenance | tee "$WORK/maint.txt"
echo

# The kind, the axis, the threshold and the rate, stated as numbers.
grep -qF -- '  the kinds: repair, seal, reclaim, build, secure, evacuate, maintenance; JOB_KIND_COUNT is 7' "$WORK/maint.txt" \
  || fail "the seventh kind is not in the enum and the register"
grep -qF -- "  the axis: a system's wear (0 fresh .. 1 the deferred-maintenance failure); the threshold MAINTENANCE_DUE is 0.20" "$WORK/maint.txt" \
  || fail "the axis (wear) or the threshold (MAINTENANCE_DUE) is not stated"
grep -qF -- '  the party: up to MAINTENANCE_TEAM_MAX 2 standing hands; MAINTENANCE_HOURS_PER_SYSTEM 240 h of one engineer restores a wholly worn system (MAINTENANCE_DAYS 10)' "$WORK/maint.txt" \
  || fail "the rule that connects crew-hours to the clock is not stated"
# The connection: the failure odds the wear sets, rising to certainty as the clock approaches 1.
grep -qF -- '  the odds it moves: MaintenanceOdds(wear) = 1 / ((1 - wear) * MAINTENANCE_DAYS): wear 0.00 -> 0.1000, 0.20 -> 0.1250, 0.50 -> 0.2000, 0.90 -> 1.0000, 1.00 -> 1.0000' "$WORK/maint.txt" \
  || fail "the odds curve is not the one the clock sets"
# The effect, both ways: the same system, maintained and not, at the end of fifteen days.
grep -qE '^  maintained: health 1\.00, wear peak 0\.20 \(the clock never crossed 1\), the failure odds peak 0\.1250, [0-9]+ maintenance job\(s\) raised$' "$WORK/maint.txt" \
  || fail "the maintained system is not shown whole with the wear bounded"
grep -qE '^  neglected \(no spare parts\): health 0\.60, wear peak 1\.00, the failure odds peak 1\.0000, [0-9]+ maintenance job\(s\) raised, failed for want of maintenance: yes$' "$WORK/maint.txt" \
  || fail "the neglected system is not shown failing for want of maintenance"
# The order, and the authority surface it travels: offered per recipient, pushed to the head, and
# giveable by the officer of the deck like any other class.
grep -qE '^  offered to .*: pill [0-9]+ "overhaul the sensors" -> the sensors \(wear 0\.50\)$' "$WORK/maint.txt" \
  || fail "the maintenance order is not offered to the engineer"
grep -qE '^  dispatched: given 1, job [0-9]+, the job.s place in the queue -1 \(the head\)$' "$WORK/maint.txt" \
  || fail "the order did not push the maintenance job to the head of the queue"
grep -qE '^  the officer of the deck \(.*\) may give it to a senior \(.*, rank [0-9]+\): 1$' "$WORK/maint.txt" \
  || fail "the officer of the deck may not give the maintenance order"
# The save: the wear clock is saved (the format moved 63 -> 64), the job round-trips, the older save
# does not load -- the cost named.
grep -qE '^  format 64 \(63 -> 64 this pass: the wear clock is saved, so an older save does not load\); the blob is [0-9]+ bytes and loads 1; the maintenance job round-trips \(1\), the party is re-derived$' "$WORK/maint.txt" \
  || fail "the wear clock or the maintenance job did not round-trip, or the format did not move"

echo
echo "==> maintenance: the falsification (the job runs and the wear does not move; the test must fail)"
cp -r module/ship "$WORK/shipmod"
# Plant: the maintenance work loop computes no gain, so the job is raised and worked and the clock
# never moves -- exactly the "job-shaped decoration" the brief names.
sed -i 's/gain = std::min(gain, sys.wear);/gain = 0.0f; \/\/ planted: the maintenance runs and changes nothing/' "$WORK/shipmod/ship_core.cpp"
grep -q 'planted: the maintenance runs and changes nothing' "$WORK/shipmod/ship_core.cpp" || fail "the wear plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod/ship_core.cpp" -o "$WORK/test_broken"
if "$WORK/test_broken" >"$WORK/broken.txt" 2>&1; then
  echo "FAIL  the test passed with a maintenance that changes nothing: the check has no teeth" >&2
  exit 1
fi
grep -q 'FAIL  maintenance: the seventh kind' "$WORK/broken.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken.txt" | grep 'FAIL  maintenance: the seventh kind' || true
echo "    the broken copy fails on the seventh kind; the tree passes"

echo
echo "PASS  the seventh kind is in the enum and the queue; the axis is wear and the threshold is 0.20;"
echo "      a maintained system stays whole with the clock bounded while an unmaintained one fails for"
echo "      want of maintenance; the order is offered per recipient, pushed to the head, and giveable"
echo "      from the deck; the wear clock is saved (63 -> 64) and the job round-trips; and the guard"
echo "      fails on a maintenance that changes nothing"
