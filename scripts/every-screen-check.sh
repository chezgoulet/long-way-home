#!/usr/bin/env bash
# The screens the inventory marked missing or thin, and were built or thickened
# (docs/evidence/every-screen.md, rows 19-23 of docs/evidence/the-missing-screens.md):
#
#   * the month report editor (row 20), the job-queue board (row 19) and the chart you can
#     work (row 23), all command's, driven through their own screens and checked against the
#     two-lock model -- a post officer is refused by name, whoever commands is not;
#   * the tricorder survey (row 22) and the beacon's choices as keys on Operations (row 21).
#
# Two headless runs (g_shipTest 69 and 70), then the accessibility measurement on the
# screenshots the engine wrote (scripts/screens-a11y.py).
#
#   scripts/every-screen-check.sh
#
# Writes under build/g3-home only.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HOME_DIR="$ROOT/build/g3-home"
GAME_DIR="$HOME_DIR/baseEF"
PALETTE="$ROOT/../upstream/efgame/src/game/q_math.cpp"
[ -d "$ROOT/build/baseEF" ] || { echo "no game data at build/baseEF -- run scripts/playtest-host-setup.sh" >&2; exit 1; }
command -v xvfb-run >/dev/null 2>&1 || { echo "xvfb-run not found" >&2; exit 1; }
[ -f "$PALETTE" ] || { echo "no palette at $PALETTE -- run scripts/bootstrap-upstream.sh" >&2; exit 1; }

fail() { echo "FAIL  $1"; exit 1; }
pass() { echo "PASS  $1"; }

run() {
  local test="$1"
  find "$GAME_DIR" -maxdepth 1 -name '*.pid' -delete
  SDL_AUDIODRIVER=dummy timeout 200 xvfb-run -a "$ROOT/scripts/run-engine.sh" --home-dir "$HOME_DIR" \
      +set s_useOpenAL 0 +set g_ship 1 +set g_shipTest "$test" +map tour/deck04 >"$HOME_DIR/every-screen-$test.out" 2>&1 || true
}

# ---- the report editor, the job board, and the two-lock on both ------------------------------
echo "==> the month report editor and the job-queue board, and the two-lock"
run 69
grep -hE '^SHIP: (report|jobs) test' "$HOME_DIR/every-screen-69.out" | sed 's/^SHIP: /    /' || true

grep -q 'report test: a post officer, may command 0' "$HOME_DIR/every-screen-69.out" \
  || fail "the post officer did not read as not-commanding (see $HOME_DIR/every-screen-69.out)"
grep -q "the post officer's strike was refused: \"the report is the commanding officer's to write\"; struck lines 0" \
  "$HOME_DIR/every-screen-69.out" || fail "a post officer edited the report, or the refusal is not the named one"
grep -q "the post officer's reorder was refused: \"priority is command's to set\"" "$HOME_DIR/every-screen-69.out" \
  || fail "a post officer set the queue's order, or the refusal is not the named one"
pass "a post officer opens both screens and every act on them is refused, by name, changing nothing"

grep -qE 'report test: in command, may command 1' "$HOME_DIR/every-screen-69.out" \
  || fail "whoever commands did not read as commanding"
grep -qE 'report test: in command, struck 1, softened 1; the diff is "- .+' "$HOME_DIR/every-screen-69.out" \
  || fail "the report editor did not strike and soften, or kept no diff"
grep -q 'report test: signed to the crew yes; a fresh draft is open yes' "$HOME_DIR/every-screen-69.out" \
  || fail "the report was not signed to the crew, or no fresh draft opened"
pass "whoever commands strikes a line, softens a number, and signs it to the crew -- the record keeps the diff"

grep -qE 'jobs test: in command, [0-9]+ job\(s\), 1 build; the queue reads "[0-9]+\|repair\|' "$HOME_DIR/every-screen-69.out" \
  || fail "the job board did not reorder, or the build job was not ordered"
pass "whoever commands sets the queue's order and orders a build -- the board of the work, as its own face"

# ---- the chart, the survey, and the beacon's choices ------------------------------------------
echo
echo "==> the chart you can work, the tricorder survey, and the beacon's choices"
run 70
grep -hE '^SHIP: (chart|survey|beacon) test' "$HOME_DIR/every-screen-70.out" | sed 's/^SHIP: /    /' || true

grep -qE 'chart test: the chart reads "0\|1\|.+' "$HOME_DIR/every-screen-70.out" \
  || fail "the chart did not publish the sector, or does not mark where the ship is"
grep -qE 'chart test: the forecast reads "BEACON 1' "$HOME_DIR/every-screen-70.out" \
  || fail "the chart carries no forecast"
grep -q 'chart test: setting the course by key left it at 1' "$HOME_DIR/every-screen-70.out" \
  || fail "setting the course from the chart did not reach the ship"
pass "the chart is workable: the sector, the forecast, and setting a course by key from the screen"

grep -qE 'survey test: the survey reads "SITE\|.+' "$HOME_DIR/every-screen-70.out" \
  || fail "the survey did not publish what a scan may be spent on"
grep -qE 'survey test: the last reading is "deck 1: air .+life support nominal"; the charge is [0-9]+%' "$HOME_DIR/every-screen-70.out" \
  || fail "a scan of a compartment did not read back, or the shared charge did not fall"
pass "the survey is a decision: a scan of the site or a compartment, out of one shared charge, and the reading returns"

grep -qE 'beacon test: at beacon [0-9]+ the choices read ".+hail.+' "$HOME_DIR/every-screen-70.out" \
  || fail "the beacon's choices are not published to the Operations console"
grep -q 'beacon test: hail by key wrote to the log: yes' "$HOME_DIR/every-screen-70.out" \
  || fail "the hail key at Operations did not reach the ship"
pass "the beacon's choices are keys on Operations now, and the hail reaches the ship"

# ---- the rendered screens, photographed -------------------------------------------------------
echo
echo "==> the rendered screens"
for shot in lwh_report_locked lwh_report lwh_jobs_locked lwh_jobs lwh_chart lwh_survey lwh_ops_beacon; do
  [ -f "$GAME_DIR/screenshots/$shot.tga" ] || fail "no screenshot of $shot"
  echo "      $GAME_DIR/screenshots/$shot.tga"
done
pass "every new screen was rendered and photographed"

# ---- contrast and rendered type height, measured against what each colour sits on -------------
echo
echo "==> accessibility: contrast and rendered type height"
if ! python3 "$ROOT/scripts/screens-a11y.py" --palette "$PALETTE" --shots "$GAME_DIR/screenshots" >"$HOME_DIR/every-screen-a11y.out" 2>&1; then
  sed 's/^/    /' "$HOME_DIR/every-screen-a11y.out"
  fail "the accessibility measurement did not pass (see $HOME_DIR/every-screen-a11y.out)"
fi
sed 's/^/    /' "$HOME_DIR/every-screen-a11y.out"

echo
echo "PASS  the missing screens are built, the thin ones thickened, and the two-lock and accessibility hold"
