#!/usr/bin/env bash
# The affinities record and its two entry points (Stage D slice 1; docs/affinities-and-allegiance.md).
# This is the demonstration and the falsification.
#
#   scripts/affinity-check.sh
#
# Part one -- the demonstration. The unit test suite runs, and `test_ship_core --affinities` prints
# the observable criterion: one event, two crew, the declared affinity that explains each delta; the
# liked program the schedule picks in a recreation hour; and the loved task the person asks for from
# the job queue. The record's count, the authored rows, and the two items the document could not
# verify (Seven's music studies, Chakotay's boxing) are printed too, so a silent regression in the
# authored rows fails the check.
#
# Part two -- the falsification, twice: a valence fold that ignores dislikes must fail the
# same-event test, and a schedule that stops picking the liked program must fail the choice test.
# A guard that cannot fail for the reason we care about is decoration.
#
# Static host: no engine and no game data; the model is exercised directly, so this runs anywhere a
# compiler does. The in-engine observation of the program on the deck is not this check and is named
# as deferred in docs/evidence/affinities.md.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

fail() { echo "FAIL  $1" >&2; exit 1; }

echo "==> affinities: the unit test suite, built here"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp module/ship/scenario_core.cpp -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> affinities: the record and the two entry points (test_ship_core --affinities)"
"$WORK/test_ship_core" --affinities | tee "$WORK/affinities.txt"
echo

# The record: 141 crew, a few strong preferences each, and authored rows beside generated ones.
grep -qE '^  the record: 141 crew, [0-9]+ likes, [0-9]+ dislikes, [0-9]+ pursuits, [0-9]+ authored rows$' "$WORK/affinities.txt" \
  || fail "the record is not the whole crew, or is not bounded"
grep -qF -- 'AUTHORED: Kathryn Janeway -- likes "coffee"' "$WORK/affinities.txt" \
  || fail "Janeway's authored coffee is missing"
grep -qF -- 'AUTHORED: Harry Kim -- likes "clarinet quartets"' "$WORK/affinities.txt" \
  || fail "Kim's authored clarinet is missing"
grep -qF -- 'AUTHORED: The Doctor -- likes "Photons Be Free"' "$WORK/affinities.txt" \
  || fail "the Doctor's authored holonovel is missing"
grep -qF -- "AUTHORED: B'Elanna Torres -- allegiance the Maquis" "$WORK/affinities.txt" \
  || fail "Torres' authored Maquis allegiance is missing"
grep -qF -- "NOT AUTHORED: Seven's music studies (absent, as required), Chakotay's boxing (absent, as required)" "$WORK/affinities.txt" \
  || fail "the two unverified canon items were authored, or the record cannot say they were not"

# The criterion: one event, two crew, one declared like and one declared dislike, and the mark the
# bond fold reads. The two lines must name the same event and opposite signs.
grep -qE '^  EVENT: ".*", base valence [-+][0-9]+\.[0-9]{2}, the same order to both$' "$WORK/affinities.txt" \
  || fail "the same event is not shown landing on two crew"
grep -qE '^  CREW A: .* -- likes ".*" \([-+][0-9]+\.[0-9]{2}\): effective [-+][0-9]+\.[0-9]{2}, mark [-+][0-9]+\.[0-9]{2}, bond [-+][0-9]+\.[0-9]{2}$' "$WORK/affinities.txt" \
  || fail "the liker's mark is not shown with its declared affinity and the bond fold"
grep -qE '^  CREW B: .* -- dislikes ".*" \([-+][0-9]+\.[0-9]{2}\): effective -[0-9]+\.[0-9]{2}, mark -[0-9]+\.[0-9]{2}, bond -[0-9]+\.[0-9]{2}$' "$WORK/affinities.txt" \
  || fail "the disliker's mark is not shown negative, with its declared affinity and the bond fold"

# Choice: the schedule picks the liked program, and the queue has a job the person asks for.
grep -qE '^  PROGRAM: .* -- recreation on deck 6, program ".*" \(liked [-+][0-9]+\.[0-9]{2}\)$' "$WORK/affinities.txt" \
  || fail "no liked program is shown picked in a recreation hour"
grep -qE '^  TASK: .* asks for job [0-9]+ \([a-z]+\); the queue.s first repair is job [0-9]+$' "$WORK/affinities.txt" \
  || fail "no loved task is shown asked for from the queue"

echo
echo "==> affinities: falsification one (the valence fold ignores dislikes; the same-event test must fail)"
cp -r module/ship "$WORK/shipmod_dislike"
sed -i 's/if (SameAffinityThing(a.thing, thing)) v -= a.strength;/if (SameAffinityThing(a.thing, thing)) v -= 0.0f; \/\/ planted: dislikes do nothing/' \
  "$WORK/shipmod_dislike/ship_core.cpp"
grep -q 'planted: dislikes do nothing' "$WORK/shipmod_dislike/ship_core.cpp" || fail "the dislike plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod_dislike" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod_dislike/ship_core.cpp" module/ship/scenario_core.cpp -o "$WORK/test_broken_dislike"
if "$WORK/test_broken_dislike" >"$WORK/broken_dislike.txt" 2>&1; then
  fail "the suite passed with a valence fold that ignores dislikes: the check has no teeth"
fi
grep -q 'FAIL  a like raises and a dislike lowers' "$WORK/broken_dislike.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken_dislike.txt" | grep 'FAIL  a like raises and a dislike lowers' || true
echo "    the broken copy fails on the same-event test; the tree passes"

echo
echo "==> affinities: falsification two (the schedule stops picking the liked program; the choice test must fail)"
cp -r module/ship "$WORK/shipmod_choice"
sed -i 's/c.program = (a == ACT_RECREATION \&\& c.deck == HOLODECK_RECREATION_DECK) ? PreferredProgram(c) : std::string();/c.program.clear(); \/\/ planted: the schedule no longer picks the liked program/' \
  "$WORK/shipmod_choice/ship_core.cpp"
grep -q 'planted: the schedule no longer picks the liked program' "$WORK/shipmod_choice/ship_core.cpp" || fail "the choice plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod_choice" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod_choice/ship_core.cpp" module/ship/scenario_core.cpp -o "$WORK/test_broken_choice"
if "$WORK/test_broken_choice" >"$WORK/broken_choice.txt" 2>&1; then
  fail "the suite passed with a schedule that does not pick the liked program: the check has no teeth"
fi
grep -q 'FAIL  choice: the liked program and the loved task are selected' "$WORK/broken_choice.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken_choice.txt" | grep 'FAIL  choice: the liked program and the loved task are selected' || true
echo "    the broken copy fails on the choice test; the tree passes"

echo
echo "PASS  the record is generated for the whole crew and authored where canon speaks; the same"
echo "      event lands positively on the one who likes it and negatively on the one who dislikes it,"
echo "      through the one mark writer and the unchanged bond fold; the schedule picks the liked"
echo "      program and the queue has the task the person asks for; and the guard fails on a valence"
echo "      fold that ignores dislikes and on a schedule that does not pick the program"
