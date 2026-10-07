#!/usr/bin/env bash
# Runs the G0 checks in one command.
#
#   scripts/check.sh --source-map PATH --script-corpus PATH [--build-root DIR]
#
# --source-map     a shipped .map file, used to build the validator's clean fixture
# --script-corpus  a directory of shipped ICARUS scripts (.txt), used for the corpus pass
# --build-root     where bootstrap-upstream.sh built things (default: ../upstream)
#
# Shipped content is supplied rather than vendored: none of it belongs in this repository.

set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"


SOURCE_MAP=""; SCRIPT_CORPUS=""; BUILD_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/../upstream"
while [ $# -gt 0 ]; do
  case "$1" in
    --source-map)    SOURCE_MAP="$2"; shift 2 ;;
    --script-corpus) SCRIPT_CORPUS="$2"; shift 2 ;;
    --build-root)    BUILD_ROOT="$2"; shift 2 ;;
    *) echo "unknown argument: $1"; exit 2 ;;
  esac
done
[ -n "$SOURCE_MAP" ] && [ -n "$SCRIPT_CORPUS" ] || { echo "usage: check.sh --source-map PATH --script-corpus PATH"; exit 2; }
for tool in "$BUILD_ROOT/ibize-build/ibize" "$BUILD_ROOT/ibi-dump-build/ibi-dump"; do
  [ -x "$tool" ] || { echo "$tool not built -- run scripts/bootstrap-upstream.sh first" >&2; exit 1; }
done

echo "==> checks that need no game content"
"$ROOT/scripts/test.sh"
echo

IBIZE="$BUILD_ROOT/ibize-build/ibize"
IBI_DUMP="$BUILD_ROOT/ibi-dump-build/ibi-dump"
DICT="$BUILD_ROOT/entities.json"

echo "==> entity dictionary"
# Our own adopted classes (docs/evidence/harvest-b-rpgx-classes.md) are documented here so the
# validator covers what our maps may place, whether or not the retail defs are present.
LWH_DEF="$ROOT/tools/entitydict/lwh_entities.def"
if [ -f "$BUILD_ROOT/SP_entities.def" ] && [ -f "$BUILD_ROOT/HM_entities-def.txt" ]; then
  python3 "$ROOT/tools/entitydict/entitydict.py" parse \
    --def "$BUILD_ROOT/SP_entities.def" --def "$BUILD_ROOT/HM_entities-def.txt" --def "$LWH_DEF" --json "$DICT"
else
  python3 "$ROOT/tools/entitydict/entitydict.py" parse --def "$LWH_DEF" --json "$DICT"
  echo "    (put SP_entities.def and HM_entities-def.txt in $BUILD_ROOT for the full class check)"
fi

echo
echo "==> validator negative tests"
python3 "$ROOT/tools/validator/tests/negative_tests.py" \
  --source-map "$SOURCE_MAP" --script-corpus "$SCRIPT_CORPUS" \
  --ibize "$IBIZE" --ibi-dump "$IBI_DUMP" ${DICT:+--entitydict "$DICT"}

echo
echo "==> compiler corpus pass (every shipped script: compile, then read back)"
python3 - "$IBIZE" "$IBI_DUMP" "$SCRIPT_CORPUS" <<'PY'
import os, subprocess, sys
from concurrent.futures import ThreadPoolExecutor
import tempfile
ibize, dump, corpus = sys.argv[1], sys.argv[2], sys.argv[3]
work = tempfile.mkdtemp(prefix="lwh-check-")
scripts = sorted(os.path.join(r, f) for r, _, fs in os.walk(corpus) for f in fs if f.lower().endswith(".txt"))

# NOTE: the interpreter reports errors with printf(), i.e. on STDOUT, while the tokenizer reports
# through the error callback on stderr. A check that reads only stderr is blind to the majority of
# failures -- which is exactly how an earlier version of this script reported a false all-clear.
def one(i_and_s):
    i, s = i_and_s
    o = os.path.join(work, f"check-{i}.IBI")
    r = subprocess.run([ibize, s, o], capture_output=True, timeout=30)
    msg = (r.stdout + r.stderr).decode(errors="replace").strip().replace("\n", " ")
    if r.returncode != 0:
        return ("compile", s, msg[:100])
    d = subprocess.run([dump, o], capture_output=True, timeout=30)
    if not d.stdout.decode(errors="replace").startswith("OK\t"):
        return ("dump", s, "")
    return ("ok", s, "")

with ThreadPoolExecutor(max_workers=8) as ex:
    results = list(ex.map(one, enumerate(scripts)))
ok = sum(1 for k, _, _ in results if k == "ok")
compile_err = [r for r in results if r[0] == "compile"]
dump_err = [r for r in results if r[0] == "dump"]

# The GDK's corpus contains eight files the official compiler rejects: the three scripts with named
# causes in docs/evidence/g0-script-compiler.md (voy1/scene7.TXT, voy1/scene10.TXT, voy5/beamstart.TXT)
# and five files that are not scripts at all (sound tables, configuration, a directory list and editor
# backups). That state is known, documented and permanent, so it is written here as the expected set:
# the exit is 0 when the failures are exactly these, and non-zero when anything else appears -- which
# is what makes the exit status mean something. A file in the list that now compiles is also a change,
# and is reported.
EXPECTED_REJECTIONS = {
    "borg6/setup.txt",
    "dn1/start.txt",
    "dn1/startbakup.txt",
    "validdirs.txt",
    "voy1/scene10.TXT",
    "voy1/scene7.TXT",
    "voy4/ordermunro.bak.txt",
    "voy5/beamstart.TXT",
}
rejected = {os.path.relpath(r[1], corpus).replace(os.sep, "/") for r in compile_err}
unexpected = sorted(rejected - EXPECTED_REJECTIONS)
missing = sorted(EXPECTED_REJECTIONS - rejected)

print(f"    {len(scripts)} files: {ok} compiled and read back, "
      f"{len(compile_err)} rejected by the compiler, {len(dump_err)} read-back failure(s)")
for kind, path, msg in (compile_err + dump_err)[:20]:
    rel = os.path.relpath(path, corpus)
    print(f"      {kind}: {rel}" + (f"  |  {msg}" if msg else "  |  (no diagnostic printed)"))
if len(compile_err) + len(dump_err) > 20:
    print(f"      ... and {len(compile_err) + len(dump_err) - 20} more")

bad = False
if dump_err:
    print(f"    {len(dump_err)} read-back failure(s): a compiled script the game's own reader cannot read")
    bad = True
if unexpected:
    print("    unexpected rejection(s) -- a new fault, or the corpus has changed:")
    for rel in unexpected:
        print(f"      {rel}")
    bad = True
if missing:
    print("    expected rejection(s) that now compile -- the known set has changed:")
    for rel in missing:
        print(f"      {rel}")
    bad = True
if not bad:
    print(f"    the {len(EXPECTED_REJECTIONS)} rejections are exactly the known, documented set")
import shutil
shutil.rmtree(work, ignore_errors=True)
sys.exit(1 if bad else 0)
PY
