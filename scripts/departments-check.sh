#!/usr/bin/env bash
# The nine departments, and the complement as hull data (the owner's rulings on rows 3.3 and 8.53 of
# docs/designs-against-code.md, 2026-10-12: "NINE, as designed" and "HULL DATA, as the design says";
# docs/crew-manifest.md, docs/design-north-star.md).
#
#   scripts/departments-check.sh
#
# Part one -- the demonstration. The unit test suite runs, and `test_ship_core --departments` prints
# the nine departments, their names, their sizes (adding to the complement) and the two complement
# start states both loading (docs/evidence/the-departments-and-the-complement.md). The lines asserted
# here are the acceptance, so a silent regression fails the check.
#
# Part two -- the falsification, one plant per ruling. The same test is compiled against a broken copy
# of ship_core.cpp in which (a) one department carries a name that is not in the roster, and must FAIL
# on the nine; then (b) Complement ignores the ship's own data and must FAIL on the complement. A
# guard that cannot fail for the reason we care about is decoration.
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

echo "==> departments: the unit test suite, built here"
"$CXX" -std=c++17 -g -I module/ship -I module/ui \
  tests/ship/test_ship_core.cpp module/ship/ship_core.cpp -o "$WORK/test_ship_core"
"$WORK/test_ship_core" >/dev/null

echo
echo "==> departments: the demonstration (test_ship_core --departments)"
"$WORK/test_ship_core" --departments | tee "$WORK/departments.txt"
echo

# The nine, by the roster's own names, each with a positive size.
for name in "Command" "Operations" "Engineering" "Security and tactical" "Science" \
            "Medical" "Flight and shuttlebay" "Support and logistics" "Civilians and attached"; do
  grep -qE "^    ${name}[[:space:]]+[0-9]+ " "$WORK/departments.txt" || fail "the roster does not name $name"
done
# The sizes add to the complement.
grep -q '^  the sizes add to 141, the complement$' "$WORK/departments.txt" \
  || fail "the nine department sizes do not add to the complement"
# The one lookup a staff field uses.
grep -q '^  one lookup: DepartmentByName("security") = 3, DepartmentName -> Security and tactical$' "$WORK/departments.txt" \
  || fail "DepartmentByName does not resolve a roster name to its department"
# The complement is hull data, and two start states with different complements both load.
grep -q '^  start state \[0\] CANON.*complement 141 (state names  0), 141 crew, 141 fit, loads 1$' "$WORK/departments.txt" \
  || fail "the canon default does not leave the hull's own complement"
grep -q '^  start state \[3\] SKELETON.*complement  96 (state names 96), 96 crew, 96 fit, loads 1$' "$WORK/departments.txt" \
  || fail "a start state that names its own complement did not field it and load"

echo
echo "==> departments: the falsification (a department not in the roster; the nine must fail)"
cp -r module/ship "$WORK/shipmod"
# Plant: Science is named with the old five-department word, which the roster does not have.
sed -i 's/"Security and tactical", "Science",/"Security and tactical", "Sciences",/' "$WORK/shipmod/ship_core.cpp"
grep -q '"Security and tactical", "Sciences",' "$WORK/shipmod/ship_core.cpp" || fail "the department plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod/ship_core.cpp" -o "$WORK/test_broken_dept"
if "$WORK/test_broken_dept" >"$WORK/broken_dept.txt" 2>&1; then
  echo "FAIL  the test passed with a department the roster does not have: the check has no teeth" >&2
  exit 1
fi
grep -q 'FAIL  the nine departments' "$WORK/broken_dept.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken_dept.txt" | grep 'FAIL  the nine departments' || true
echo "    the broken copy fails on the nine; the tree passes"

echo
echo "==> departments: the falsification (Complement ignores its data; the complement must fail)"
cp -r module/ship "$WORK/shipmod2"
# Plant: Complement no longer reads the ship's own hull data -- it returns the one default.
sed -i 's/int Complement(const Ship &s) { return s.complement; }/int Complement(const Ship \&s) { (void)s; return COMPLEMENT_DEFAULT; }/' \
  "$WORK/shipmod2/ship_core.cpp"
grep -q 'return COMPLEMENT_DEFAULT; }' "$WORK/shipmod2/ship_core.cpp" || fail "the complement plant did not take"
"$CXX" -std=c++17 -g -I "$WORK/shipmod2" -I module/ui \
  tests/ship/test_ship_core.cpp "$WORK/shipmod2/ship_core.cpp" -o "$WORK/test_broken_comp"
if "$WORK/test_broken_comp" >"$WORK/broken_comp.txt" 2>&1; then
  echo "FAIL  the test passed with a complement that ignores its data: the check has no teeth" >&2
  exit 1
fi
grep -q 'FAIL  the complement is hull data' "$WORK/broken_comp.txt" \
  || fail "the broken copy failed for the wrong reason"
sed -n 's/^/      /p' "$WORK/broken_comp.txt" | grep 'FAIL  the complement is hull data' || true
echo "    the broken copy fails on the complement; the tree passes"

echo
echo "PASS  the roster's nine departments are the code's vocabulary, their sizes add to the complement,"
echo "      the complement is hull data a start state may vary with both states loading, and the guard"
echo "      fails for the reason each ruling is about"
