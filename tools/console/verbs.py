#!/usr/bin/env python3
"""The ship's API freshness check (docs/evidence/the-computer-api.md, Task A).

The ship's API -- the command surface the console `Svcmd_Ship_f` answers, and therefore the
enumerated set the ship's computer may answer from -- lives in one place a program can read:
`module/ship/console_api.def` (included by `module/ship/ship_core.cpp`).  This tool checks that
enumeration against the surface that keeps growing, the console itself, in **both** directions:

  * a verb the console answers and the enumeration does not carry  -> FAIL (the omission drift)
  * a verb the enumeration carries and the console does not answer -> FAIL (the stale row)

The pattern is `tools/hooks/surface.py` + `tools/hooks/check_register.py`: the register is curated,
the surface is derived, and a check fails when they drift.  This tool does not judge arguments or
stations beyond their shape; it checks *membership*, which is what the guardrail is defined against.

  tools/console/verbs.py check                  # both directions (what scripts/api-check.sh runs)
  tools/console/verbs.py derive                 # the console's verb set, one per line
  tools/console/verbs.py enumerate              # verb<TAB>args<TAB>stations, one per line
  tools/console/verbs.py check --console X --register Y   # check modified copies without touching the tree
"""

import argparse
import re
import sys
from pathlib import Path

# The console's whole command dispatch. The verb literals are inline `!Q_stricmp( cmd, "..." )` in
# one function; the derivation is tied to that rule by construction (see docs/evidence/the-computer-api.md).
_FUNC = "void Svcmd_Ship_f( void )"
_CMD_LITERAL = re.compile(r'Q_stricmp\(\s*cmd\s*,\s*"([^"]+)"')
_DEF_ROW = re.compile(r'^\s*CONSOLE_VERB\(\s*"([^"]*)"\s*,\s*"([^"]*)"\s*,\s*"([^"]*)"\s*\)\s*$', re.M)

# The canonical StationName tokens a `stations` field may name, plus the two special values.
_STATIONS = {"any", "-", "engineering", "tactical", "conn", "ops", "sickbay"}


def _function_body(text, signature):
    """The body of the top-level C function `signature`, by brace matching from its first `{`."""
    start = text.index(signature)
    open_brace = text.index("{", start)
    depth = 0
    for i in range(open_brace, len(text)):
        c = text[i]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return text[open_brace : i + 1]
    raise ValueError("unbalanced braces after %r" % signature)


def derive(path):
    """The console's verb set: every first-token command literal Svcmd_Ship_f compares against."""
    body = _function_body(Path(path).read_text(encoding="utf-8"), _FUNC)
    return sorted(set(_CMD_LITERAL.findall(body)))


def enumerate_api(path):
    """The enumeration's rows as (verb, args, stations), in table order; raises on a malformed row."""
    text = Path(path).read_text(encoding="utf-8")
    rows = _DEF_ROW.findall(text)
    # Every CONSOLE_VERB line must have parsed: a malformed one is a check failure, not a silent skip.
    declared = len(re.findall(r"^\s*CONSOLE_VERB\(", text, re.M))
    if declared != len(rows):
        raise ValueError("%d CONSOLE_VERB line(s) but only %d parsed in %s" % (declared, len(rows), path))
    seen = set()
    for verb, args, stations in rows:
        if not re.fullmatch(r"[a-z][a-z0-9]*", verb):
            raise ValueError("malformed verb %r" % verb)
        if verb in seen:
            raise ValueError("duplicate verb %r" % verb)
        seen.add(verb)
        for tok in stations.split("|"):
            if tok not in _STATIONS:
                raise ValueError("verb %r names an unknown station %r (allowed: %s)"
                                 % (verb, tok, ", ".join(sorted(_STATIONS))))
        if not args:
            raise ValueError("verb %r has an empty args field" % verb)
    return rows


def check(console, register, out):
    console_verbs = derive(console)
    rows = enumerate_api(register)
    registered = sorted({r[0] for r in rows})

    out("verbs.py: %s answers %d verb(s); %s enumerates %d" % (console, len(console_verbs), register, len(registered)))

    missing = [v for v in console_verbs if v not in registered]
    extra = [v for v in registered if v not in console_verbs]

    failed = False
    if missing:
        failed = True
        out("FAIL  the console answers and the API does not carry (%d): %s" % (len(missing), " ".join(missing)))
    if extra:
        failed = True
        out("FAIL  the API carries and the console does not answer (%d): %s" % (len(extra), " ".join(extra)))
    if failed:
        return 1
    out("PASS  every console verb is in the ship's API, and every API verb is answered by the console")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=["check", "derive", "enumerate"])
    ap.add_argument("--console", default="module/ship/g_ship.cpp")
    ap.add_argument("--register", default="module/ship/console_api.def")
    args = ap.parse_args()

    if args.mode == "derive":
        for v in derive(args.console):
            print(v)
        return 0
    if args.mode == "enumerate":
        for verb, argv, stations in enumerate_api(args.register):
            print("%s\t%s\t%s" % (verb, argv, stations))
        return 0
    try:
        return check(args.console, args.register, lambda s: print(s))
    except ValueError as e:
        print("FAIL  %s" % e)
        return 1


if __name__ == "__main__":
    sys.exit(main())
