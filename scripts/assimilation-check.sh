#!/usr/bin/env bash
# Assimilation takes the log and the access (docs/the-record-and-the-log.md): when the Borg take a
# mind, the Collective gains that person's private words, their marks and their authority, and speaks
# in their voice. A purge does not end it; a de-assimilation in the window, or a death, does.
#
#   scripts/assimilation-check.sh
#
# One half, and it needs no game data: the unit transcript (`test_ship_core --assimilation`), which
# prints one PASS line per acceptance item and exits non-zero if any fails. The rules are also in the
# suite (`TestTheAssimilation`); this is the evidence command. The in-game half is the `g_shipTest`
# case 100 on a host with the game data (`scripts/run-lwh.sh +set g_shipTest 100 +map tour/deck04`),
# named in `docs/evidence/the-assimilation.md` and not run here.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

fail() { echo "FAIL  $1"; exit 1; }

WORK="$(mktemp -d)"
trap 'rm -rf "${WORK:?}"' EXIT

echo "==> the transfer: the log, the access, and the voice"
cmake -S tests/ship -B "$WORK" -DCMAKE_BUILD_TYPE=Debug >/dev/null
cmake --build "$WORK" -j"$(nproc)" >/dev/null

"$WORK/test_ship_core" --assimilation | tee "$WORK/assimilation.txt" | sed 's/^/    /'

grep -q '^PASS  before the transfer the Collective holds 0 private entry' "$WORK/assimilation.txt" \
  || fail "the Collective held entries before anyone was taken"
grep -q '^PASS  after the transfer it holds 2, from .*: "I do not trust the captain.s numbers" and "the captain was wrong about the nebula"' "$WORK/assimilation.txt" \
  || fail "the transfer did not carry the taken person's named private entries"
grep -q '^PASS  the authority it took: sickbay yes, tactical no' "$WORK/assimilation.txt" \
  || fail "the authority transferred was not scoped to what the taken person could operate"
grep -q '^PASS  a security officer taken adds tactical: yes; engineering stays no' "$WORK/assimilation.txt" \
  || fail "the union over the taken, or the limit on a station nobody taken could work, is wrong"
grep -q '^PASS  the voice the Collective speaks in is the taken person.s own: "tuvok" (the ordinary cast gives "")' "$WORK/assimilation.txt" \
  || fail "the Collective did not speak in the taken person's own voice"
grep -q '^PASS  a purge does not end it: the Collective still holds 3 private entry' "$WORK/assimilation.txt" \
  || fail "a purge ended the transfer (the log is taken from the mind, not the disk)"
grep -q '^PASS  a de-assimilation ends it: the mind is the person.s again (yes)' "$WORK/assimilation.txt" \
  || fail "a de-assimilation did not end the Collective's access to the mind"
echo
echo "PASS  the private log and the access transfer, the Collective speaks in the taken voice, and"
echo "      a purge cannot end it while a de-assimilation can"
