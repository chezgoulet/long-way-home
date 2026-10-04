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

# Locate cmake/ninja. They are frequently installed rootlessly in a venv rather than system-wide;
# a bare `cmake: command not found` deep inside a build is a bad way to learn that.
if ! command -v cmake >/dev/null 2>&1 || ! command -v ninja >/dev/null 2>&1; then
  for candidate in "$HOME/.cache/lwh-venv/bin" "$ROOT/.venv/bin" "$ROOT/../.venv/bin" "/usr/local/bin"; do
    if [ -x "$candidate/cmake" ]; then PATH="$candidate:$PATH"; break; fi
  done
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake not found on PATH." >&2
  echo "  install it, or do this without root:" >&2
  echo "    python3 -m venv .venv && .venv/bin/pip install cmake ninja" >&2
  exit 1
fi
if ! command -v ninja >/dev/null 2>&1; then
  echo "ninja not found on PATH (cmake can use make with -G 'Unix Makefiles', or install ninja)." >&2
  exit 1
fi

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

IBIZE="$BUILD_ROOT/ibize-build/ibize"
IBI_DUMP="$BUILD_ROOT/ibi-dump-build/ibi-dump"
DICT="$BUILD_ROOT/entities.json"

echo "==> entity dictionary"
python3 "$ROOT/tools/entitydict/entitydict.py" parse \
  --def "$BUILD_ROOT/efgame/../SP_entities.def" 2>/dev/null || true
if [ -f "$BUILD_ROOT/SP_entities.def" ] && [ -f "$BUILD_ROOT/HM_entities-def.txt" ]; then
  python3 "$ROOT/tools/entitydict/entitydict.py" parse \
    --def "$BUILD_ROOT/SP_entities.def" --def "$BUILD_ROOT/HM_entities-def.txt" --json "$DICT"
else
  echo "    (put SP_entities.def and HM_entities-def.txt in $BUILD_ROOT, or the class check narrows)"
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
ibize, dump, corpus = sys.argv[1], sys.argv[2], sys.argv[3]
scripts = sorted(os.path.join(r, f) for r, _, fs in os.walk(corpus) for f in fs if f.lower().endswith(".txt"))

# NOTE: the interpreter reports errors with printf(), i.e. on STDOUT, while the tokenizer reports
# through the error callback on stderr. A check that reads only stderr is blind to the majority of
# failures -- which is exactly how an earlier version of this script reported a false all-clear.
def one(i_and_s):
    i, s = i_and_s
    o = f"/tmp/check-{i}.IBI"
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
print(f"    {len(scripts)} files: {ok} compiled and read back, "
      f"{len(compile_err)} rejected by the compiler, {len(dump_err)} read-back failure(s)")
for kind, path, msg in (compile_err + dump_err)[:20]:
    rel = os.path.relpath(path, corpus)
    print(f"      {kind}: {rel}" + (f"  |  {msg}" if msg else "  |  (no diagnostic printed)"))
if len(compile_err) + len(dump_err) > 20:
    print(f"      ... and {len(compile_err) + len(dump_err) - 20} more")
sys.exit(1 if (compile_err or dump_err) else 0)
PY
