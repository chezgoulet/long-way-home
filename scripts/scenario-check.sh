#!/usr/bin/env bash
# The declaration contract -- the gate content waits on (docs/scenario-manifest.md).
#
#   scripts/scenario-check.sh
#
# Part one -- the worked example, end to end. The manifest is validated by the same schema the
# validator uses, compiled by tools/scenario to the flat file, and the ship loader parses it and
# instantiates what it declares. The lines asserted below are the acceptance, so a silent regression
# fails the check.
#
# Part two -- the two laws, refused by the validator's own words: content that names a crew member
# (E010), and a line that does not declare its delivery direction (E011).
#
# Part three -- a bad field refused (E009), and a field the loader ignores caught: the loader's field
# list is compared to the schema's, and a planted extra field must fail the comparison.
#
# Static host: no engine and no game data, so the rendered surfaces are the playtest host's and are
# named as such in the evidence; the loader's own output is asserted here.

set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-g++}"
WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

SCEN="scenarios/deck11-gel-pack"
fail() { echo "FAIL  $1" >&2; exit 1; }

echo "==> scenario: build the loader and the unit suite"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp module/ship/scenario_core.cpp \
  -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> scenario: the manifest validates against the contract"
python3 tools/validator/validate.py "$SCEN" >"$WORK/validate.txt" 2>&1 || { cat "$WORK/validate.txt"; fail "the worked example was refused"; }
grep -q "declaration: 11 field(s) declared and checked; 11 in the contract" "$WORK/validate.txt" \
  || { cat "$WORK/validate.txt"; fail "the validator did not report the whole contract declared"; }
grep -q "0 error(s)" "$WORK/validate.txt" || fail "the worked example did not validate clean"
echo "PASS  the manifest validates: every one of the 11 contract fields declared and checked"

echo
echo "==> scenario: the declaration is compiled and the simulation does what it declares"
python3 tools/scenario/scenario_build.py "$SCEN" >/dev/null
FLAT="$SCEN/build/maps/deck11-gel-pack.scenario"
[ -f "$FLAT" ] || fail "the builder wrote no flat declaration"
"$WORK/test_ship_core" --scenario "$FLAT" >"$WORK/demo.txt" 2>&1 || { cat "$WORK/demo.txt"; fail "the loader refused the built declaration"; }
cat "$WORK/demo.txt"
assert_line() { grep -qF "$1" "$WORK/demo.txt" || fail "the transcript lacks: $1"; }
assert_line 'scenario "deck11-gel-pack": 11 declaration field(s) carried by the loader'
assert_line 'system holodecks: off'
assert_line 'system replicators: condition 1.00 -> 0.70'
assert_line 'staff: 3 post(s) required; the ship needs 22 posts fully manned'
assert_line 'warmth holodeck: a program runs for'
assert_line 'position the engineering seat -> '
assert_line 'order to the engineering seat ('
assert_line 'condolence to the medical seat ('
assert_line 'visibility deck:11:'
assert_line 'carried: 3 breeding condition(s)'
echo
echo "PASS  the loader instantiated the declaration: a system off, one under stress, staff coverage computed, warmth run, and every post resolved to a person"

echo
echo "==> scenario: the post-not-name law holds across rosters (the content is unchanged)"
grep -q 'post the engineering seat   canon: .* fictitious: Crewman' "$WORK/demo.txt" \
  || fail "the same post did not resolve to a different person under a fictitious roster"
grep -q 'order to the engineering seat (Crewman' "$WORK/demo.txt" \
  || fail "the line did not resolve to the fictitious holder"
echo "PASS  a line written by post is unchanged whether the crew is canon or entirely fictitious"

echo
echo "==> scenario: the two laws, refused by the validator's own words"
mk() { # name, extra json fragment
  local dir="$WORK/$1"; mkdir -p "$dir"
  printf '{"name":"%s","maps":[],"scripts":[],"crew":{"map":"lwhroom"}%s}\n' "$1" "$2" > "$dir/scenario.json"
  echo "$dir"
}
refuse() { # label code needle dir
  local out
  out="$(python3 tools/validator/validate.py "$4" --quiet 2>&1 || true)"
  if echo "$out" | grep -q "^$2" && echo "$out" | grep -qF "$3"; then
    echo "PASS  $1"
    echo "$out" | grep "^$2" | sed 's/^/        /'
  else
    echo "$out" | sed 's/^/        /'
    fail "$1: expected $2 naming '$3'"
  fi
}

refuse "content that names a crew member is refused (the post-not-name law)" E010 "names a crew member" \
  "$(mk names-person ',"lines":[{"to":"tuvok","delivery":"order","text":"Tuvok, seal the breach."}]')"
refuse "a line that does not declare its delivery direction is refused" E011 "declares no delivery direction" \
  "$(mk no-delivery ',"lines":[{"to":"sickbay","text":"Anything at all."}]')"
refuse "a manifest with a bad field is refused" E009 "unknown system" \
  "$(mk bad-system ',"systems":["not_a_system"]')"
refuse "an unknown manifest key is refused (a field nothing reads)" E009 "unknown manifest key" \
  "$(mk unknown-key ',"bogus_field":true')"

echo
echo "==> scenario: a field the loader ignores is caught (the field lists must agree)"
python3 tools/scenario/scenario_build.py --fields | sort > "$WORK/schema-fields.txt"
"$WORK/test_ship_core" --scenario-fields | sort > "$WORK/loader-fields.txt"
diff -u "$WORK/schema-fields.txt" "$WORK/loader-fields.txt" >/dev/null \
  || { diff -u "$WORK/schema-fields.txt" "$WORK/loader-fields.txt" || true; fail "the loader and the schema disagree on the field list"; }
echo "PASS  the loader carries every contract field, and no field the schema does not name"

echo
echo "==> scenario: the falsification -- plant a field the loader carries and the schema does not"
mkdir -p "$WORK/shipmod"
cp module/ship/scenario_core.cpp "$WORK/shipmod/scenario_core.cpp"
sed -i 's/"lines",$/"lines", "bogus_field",/' "$WORK/shipmod/scenario_core.cpp"
grep -q '"bogus_field"' "$WORK/shipmod/scenario_core.cpp" || fail "the field plant did not take"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp "$WORK/shipmod/scenario_core.cpp" \
  -o "$WORK/test_broken"
"$WORK/test_broken" --scenario-fields | sort > "$WORK/loader-broken.txt"
if diff -u "$WORK/schema-fields.txt" "$WORK/loader-broken.txt" >/dev/null; then
  fail "the falsification did not fail: a planted field went unnoticed"
fi
echo "PASS  the planted field fails the comparison, so the check has teeth"

echo
echo "==> scenario: every manifest under scenarios/ validates and builds (the nine, and the frame)"
n=0
for d in scenarios/*/; do
  [ -f "$d/scenario.json" ] || continue
  n=$((n + 1))
  python3 tools/validator/validate.py "$d" --quiet >/dev/null 2>&1 \
    || { python3 tools/validator/validate.py "$d"; fail "an authored scenario was refused: $d"; }
  python3 tools/scenario/scenario_build.py "$d" >/dev/null \
    || fail "an authored scenario did not build: $d"
  echo "    ${d%/} validated and built"
done
[ "$n" -ge 9 ] || fail "expected at least the nine-scenario slate, found $n manifest(s)"
echo "PASS  $n scenario manifest(s) validate against the contract and build"
echo
echo "all scenario checks passed"
