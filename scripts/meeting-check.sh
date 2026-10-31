#!/usr/bin/env bash
# The meeting, Stage D slice 2: the per-participant brief, the agenda, the async generator, and the
# published report (docs/staff-meetings.md; docs/the-record-and-the-log.md).
#
#   scripts/meeting-check.sh [--engine [--map NAME]]
#
# The default is static: no engine, no game data, no model and no network. It compiles the ship
# tests on the host, runs the whole suite, and prints the observable criteria:
#
#   1. a meeting plays with the model absent -- the authored skeleton is the floor;
#   2. two participants receive different briefs for the same meeting, traceably to a declared
#      affinity: different wants and silences, and opposite stances on options the skeleton declares;
#   3. the agenda is grounded in the record (the call, an instrument, a promise, a logged fact);
#   4. the published report is produced and addresses every agenda item, naming the dissent;
#   5. the same state builds the same brief, and the worker writes the same script twice.
#
# Then it falsifies twice: a stance rule that ignores affinity must fail the brief test, and a
# report that is not published must fail the report test. A guard that cannot fail for the reason we
# care about is decoration.
#
# `--engine` adds the headless in-engine check (the queue, the allocation decision end to end, and
# the meeting playing on arrival), which needs the game data and xvfb-run. It is not the static
# criterion and is not claimed when it has not been run.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

ENGINE=0
MAP="tour/deck04"
while [ $# -gt 0 ]; do
  case "$1" in
    --engine) ENGINE=1; shift ;;
    --map) MAP="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

fail() { echo "FAIL  $1" >&2; exit 1; }

echo "==> meeting: the unit test suite, built here"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp module/ship/scenario_core.cpp -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> meeting: the brief, the agenda and the report (test_ship_core --meeting)"
"$WORK/test_ship_core" --meeting | tee "$WORK/meeting.txt"
echo

grep -qF -- '== the brief, per participant: what each attendee knows, wants and will not say' "$WORK/meeting.txt" \
  || fail "the per-participant brief is not shown"
grep -qE '^  PARTICIPANT: .* -- the Maquis \(\+[0-9]+\.[0-9]{2}\)' "$WORK/meeting.txt" \
  || fail "no Maquis-weighted participant is shown with their declared allegiance"
grep -qE '^  PARTICIPANT: .* -- Starfleet \(\+[0-9]+\.[0-9]{2}\)' "$WORK/meeting.txt" \
  || fail "no Starfleet-weighted participant is shown with their declared allegiance"
grep -qE '^    wants: wants ".*" kept' "$WORK/meeting.txt" \
  || fail "no participant's want is derived from a declared like"
grep -qE '^    will not say: will not say' "$WORK/meeting.txt" \
  || fail "no participant's silence is derived from a declared dislike or allegiance"
grep -qF -- 'option 1 "The chief'"'"'s plan" (Starfleet procedure): OPPOSES -- the Maquis' "$WORK/meeting.txt" \
  || fail "the Maquis participant does not oppose the Starfleet-flavoured option, traceably"
grep -qF -- 'option 2 "Refuse the chief" (Maquis pragmatism): SUPPORTS -- the Maquis' "$WORK/meeting.txt" \
  || fail "the Maquis participant does not support the Maquis-flavoured option, traceably"
grep -qF -- 'option 1 "The chief'"'"'s plan" (Starfleet procedure): SUPPORTS -- Starfleet' "$WORK/meeting.txt" \
  || fail "the Starfleet participant does not support the Starfleet-flavoured option, traceably"
grep -qF -- 'AGENDA 1 [the call]' "$WORK/meeting.txt" \
  || fail "the agenda does not begin with the meeting's own call"
grep -qE '^  AGENDA [0-9]+ \[(an instrument|a mark|a promise|the log)\]' "$WORK/meeting.txt" \
  || fail "the agenda is not grounded in the record or the instruments"

grep -qF -- '== a meeting plays with the model absent' "$WORK/meeting.txt" \
  || fail "the model-free floor is not shown"
grep -qE '^  no generated script is loaded; the skeleton has [1-9][0-9]* outcome\(s\); outcome 1 resolved=1$' "$WORK/meeting.txt" \
  || fail "the authored skeleton did not resolve the meeting without a model"

grep -qF -- '== the published report addresses the agenda' "$WORK/meeting.txt" \
  || fail "the report is not shown"
grep -qE '^  report\(s\) 1; agenda line\(s\) [0-9]+ of [0-9]+; dissent line\(s\) [1-9][0-9]*$' "$WORK/meeting.txt" \
  || fail "the report did not address every agenda item, or named no dissent"
grep -qE '^  LOG: meeting report .*decided' "$WORK/meeting.txt" \
  || fail "no meeting report was published to the official log"

grep -qE '^  two builds: agenda [0-9]+/[0-9]+, same text and stances yes$' "$WORK/meeting.txt" \
  || fail "the brief is not deterministic from the record and the seed"

echo
echo "==> meeting: the generator is deterministic and soft without a model (tests/tools)"
python3 -m unittest tests.tools.test_meeting_worker -q
echo "    the worker writes the same script twice, and writes no script when the model is absent"

echo
echo "==> meeting: falsification one (the stance rule ignores affinity; the brief test must fail)"
cp -r module/ship "$WORK/shipmod_stance"
sed -i 's/if (score >= 0.15f) return STANCE_SUPPORTS;/return STANCE_NEUTRAL; \/\/ planted: the stance ignores affinity/' \
  "$WORK/shipmod_stance/ship_core.cpp"
grep -q 'planted: the stance ignores affinity' "$WORK/shipmod_stance/ship_core.cpp" || fail "the stance plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod_stance" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod_stance/ship_core.cpp" module/ship/scenario_core.cpp -o "$WORK/test_broken_stance"
if "$WORK/test_broken_stance" >"$WORK/broken_stance.txt" 2>&1; then
  fail "the suite passed with a stance rule that ignores affinity: the check has no teeth"
fi
grep -q 'FAIL  a brief is built per participant from marks, the log and a declared affinity' "$WORK/broken_stance.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken_stance.txt" | grep 'FAIL  a brief is built per participant' || true
echo "    the broken copy fails on the per-participant brief; the tree passes"

echo
echo "==> meeting: falsification two (the report is not published; the report test must fail)"
cp -r module/ship "$WORK/shipmod_report"
sed -i 's/PublishMeetingReport(s, brief, outcome, decidedBy, automatic, ok);/\/\/ planted: no report is published/' \
  "$WORK/shipmod_report/ship_core.cpp"
grep -q 'planted: no report is published' "$WORK/shipmod_report/ship_core.cpp" || fail "the report plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod_report" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod_report/ship_core.cpp" module/ship/scenario_core.cpp -o "$WORK/test_broken_report"
if "$WORK/test_broken_report" >"$WORK/broken_report.txt" 2>&1; then
  fail "the suite passed with no report published: the check has no teeth"
fi
grep -q 'FAIL  the published report addresses the agenda and names the dissent' "$WORK/broken_report.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken_report.txt" | grep 'FAIL  the published report' || true
echo "    the broken copy fails on the report; the tree passes"

if [ "$ENGINE" -eq 1 ]; then
  echo
  echo "==> meeting: the overlay and the allocation decision, in the engine (g_shipTest 80)"
  HOME_DIR="$ROOT/build/g3-home"
  GAME_DIR="$HOME_DIR/baseEF"
  OUT="$GAME_DIR/ship"
  [ -d "$ROOT/build/baseEF" ] || fail "no game data at build/baseEF -- run scripts/playtest-host-setup.sh"
  command -v xvfb-run >/dev/null 2>&1 || fail "xvfb-run not found"
  mkdir -p "$OUT" "$GAME_DIR/screenshots"
  rm -f "${OUT:?}/meeting.txt"
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  SDL_AUDIODRIVER=dummy timeout 180 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipMode 1 +set g_shipTest 80 +map "$MAP" \
      >"$HOME_DIR/meeting.out" 2>&1 || true
  grep -h '^SHIP: meeting test' "$HOME_DIR/meeting.out" | sed 's/^/    /' || true
  [ -f "$OUT/meeting.txt" ] || fail "the meeting run did not finish: the game stopped before the decision"
  grep -q 'brief(s) queued at the watch change' "$HOME_DIR/meeting.out" || fail "no brief was emitted at the watch change"
  grep -q 'SHIP: meeting test: watch-change:' "$HOME_DIR/meeting.out" || fail "the emitted brief was not the ordinary watch-change brief"
  grep -q 'line -> synthesis: delivery .*, known 1, exaggeration' "$HOME_DIR/meeting.out" || fail "the synthesis seam did not carry a delivery direction with the line"
  grep -q 'allocation outcome applied=1; holodecks 100%, shields 0% (provenance the player)' "$HOME_DIR/meeting.out" || fail "a person's allocation outcome was not applied end to end"
  grep -q 'set as the ship (automatic) applied=0' "$HOME_DIR/meeting.out" || fail "the meeting was allowed to set an allocation as the ship"
  echo "    PASS  the engine run above: the queue, the decision, and the skeleton on arrival"
fi

echo
echo "PASS  a meeting plays with the model absent; two participants receive different briefs for the"
echo "      same meeting, traceably to a declared affinity; the agenda is grounded in the record and"
echo "      the published report addresses every item and names the dissent; the brief and the"
echo "      generator are deterministic; and the falsifications fail for the reason they should."
