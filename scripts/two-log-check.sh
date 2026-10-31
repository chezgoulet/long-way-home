#!/usr/bin/env bash
# The two logs, and what a purge leaves behind -- Stage D slice 3 (docs/staff-meetings.md;
# docs/the-record-and-the-log.md).
#
#   scripts/two-log-check.sh
#
# Static: no engine, no game data, no model and no network. It compiles the ship tests on the host,
# runs the whole suite, and prints the observable criterion:
#
#   a meeting publishes its report into the official log; the people who were not in the room and could
#   read its scope carry a MEM_LOG mark (MEM_MEETING); a purge removes the report and leaves those
#   marks held and orphaned -- not erased, and not silently reattributed.
#
# Then it falsifies twice: a purge that does not orphan must fail the test, and a meeting that
# writes no mark must fail it. A guard that cannot fail for the reason we care about is decoration.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

fail() { echo "FAIL  $1" >&2; exit 1; }

echo "==> two logs: the unit test suite, built here"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp module/ship/scenario_core.cpp -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> two logs: the report is a document, the marks are the person's (test_ship_core --two-logs)"
"$WORK/test_ship_core" --two-logs | tee "$WORK/two-logs.txt"
echo

grep -qF -- "== before the purge: the meeting's report is published" "$WORK/two-logs.txt" \
  || fail "no meeting is shown publishing its report"
grep -qE '^  report\(s\) in the official log: [1-9][0-9]*$' "$WORK/two-logs.txt" \
  || fail "the meeting published no report into the official log"
grep -qE '^  reader: .* \(crew [0-9]+, not in the room\)$' "$WORK/two-logs.txt" \
  || fail "no crew member outside the room is shown reading the report"
grep -qE '^  reader.s mark before: event 11 \(a meeting\); provenance read it in the published report; orphaned 0$' \
  "$WORK/two-logs.txt" \
  || fail "the reader does not hold a MEM_LOG meeting mark whose report still exists"

grep -qF -- '== after the purge: the report is gone, the mark remains' "$WORK/two-logs.txt" \
  || fail "the purge is not shown"
grep -qE '^  purge ran: 1; official log entries: 0$' "$WORK/two-logs.txt" \
  || fail "the purge did not empty the published log"
grep -qE '^  meeting report\(s\) remaining: 0$' "$WORK/two-logs.txt" \
  || fail "the published report is still there after the purge"
grep -qE '^  reader.s mark after: held 1; event 11 \(a meeting\); provenance read it in a published report that has since been purged; orphaned 1$' \
  "$WORK/two-logs.txt" \
  || fail "the mark did not survive the purge, or it was not marked orphaned"
grep -qF -- 'not reattributed: source read-it-in-the-log, not saw-it and not told' "$WORK/two-logs.txt" \
  || fail "the mark was silently reattributed by the purge"
grep -qE '^  a brief still builds after the purge: [1-9][0-9]* present, [1-9][0-9]* agenda item\(s\)$' \
  "$WORK/two-logs.txt" \
  || fail "a reader of the record could not build the brief once the report was gone"
grep -qF -- 'save and reload carry the orphaned mark and no report: log 0, orphan 1, identical 1' \
  "$WORK/two-logs.txt" \
  || fail "the orphaned mark and the missing report do not survive a save and load"
grep -qF -- 'all demonstrations passed' "$WORK/two-logs.txt" \
  || fail "the demonstration itself reported a failure"

echo
echo "==> two logs: falsification one (the purge does not orphan; the test must fail)"
cp -r module/ship "$WORK/shipmod_orphan"
sed -i 's/if (m.source == MEM_LOG) m.orphaned = true;/\/\* planted: the purge does not orphan \*\/;/' \
  "$WORK/shipmod_orphan/ship_core.cpp"
grep -q 'planted: the purge does not orphan' "$WORK/shipmod_orphan/ship_core.cpp" \
  || fail "the orphan plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod_orphan" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod_orphan/ship_core.cpp" module/ship/scenario_core.cpp \
  -o "$WORK/test_broken_orphan"
if "$WORK/test_broken_orphan" >"$WORK/broken_orphan.txt" 2>&1; then
  fail "the suite passed with a purge that erases the mark's citation: the check has no teeth"
fi
grep -q 'FAIL  a purge removes the meeting.s report and leaves its MEM_LOG marks orphaned' \
  "$WORK/broken_orphan.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken_orphan.txt" \
  | grep 'FAIL  a purge removes the meeting' || true
echo "    the broken copy fails on the orphaned mark; the tree passes"

echo
echo "==> two logs: falsification two (the meeting writes no mark; the test must fail)"
cp -r module/ship "$WORK/shipmod_mark"
sed -i 's/Remember(s, i, MEM_MEETING, -1, MEM_LOG, EventValence(MEM_MEETING));/\/\* planted: the meeting writes no mark \*\//' \
  "$WORK/shipmod_mark/ship_core.cpp"
grep -q 'planted: the meeting writes no mark' "$WORK/shipmod_mark/ship_core.cpp" \
  || fail "the mark plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod_mark" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod_mark/ship_core.cpp" module/ship/scenario_core.cpp \
  -o "$WORK/test_broken_mark"
if "$WORK/test_broken_mark" >"$WORK/broken_mark.txt" 2>&1; then
  fail "the suite passed with a meeting that writes no mark: the check has no teeth"
fi
grep -q 'FAIL  a purge removes the meeting.s report and leaves its MEM_LOG marks orphaned' \
  "$WORK/broken_mark.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken_mark.txt" \
  | grep 'FAIL  a purge removes the meeting' || true
echo "    the broken copy fails on the missing mark; the tree passes"

echo
echo "PASS  a meeting publishes its report into the official log and the people outside the room who"
echo "      could read it carry it as a MEM_LOG mark; a purge removes the report and leaves those marks held,"
echo "      orphaned and not reattributed; a reader tolerates the missing report and says so; and"
echo "      the falsifications fail for the reason they should."
