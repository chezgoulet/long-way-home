#!/usr/bin/env python3
"""The ship's API freshness check (docs/evidence/the-computer-api.md, Task A), and the console
ladder's walk (docs/evidence/console-ladder-and-recall.md, Task B).

The ship's API -- the command surface the console `Svcmd_Ship_f` answers, and therefore the
enumerated set the ship's computer may answer from -- lives in one place a program can read:
`module/ship/console_api.def` (included by `module/ship/ship_core.cpp`).  This tool checks that
enumeration against the surface that keeps growing, the console itself, in **both** directions:

  * a verb the console answers and the enumeration does not carry  -> FAIL (the omission drift)
  * a verb the enumeration carries and the console does not answer -> FAIL (the stale row)

The pattern is `tools/hooks/surface.py` + `tools/hooks/check_register.py`: the register is curated,
the surface is derived, and a check fails when they drift.  This tool does not judge arguments or
stations beyond their shape; it checks *membership*, which is what the guardrail is defined against.

**And it walks the dispatch ladder**, because a set cannot see a name claimed twice.  `derive()`
used to collect the verb literals with `sorted(set(...))`, which collapses a duplicate before the
comparison: if `Svcmd_Ship_f` dispatched on one verb in two branches, only one could ever run, and
the set was blind to it (the defect docs/audit-whole-project.md, section 2.1 recorded for `recall`).
The walk reports a verb claimed more than once, in ladder order, and distinguishes two cases:

  * **both reachable** -- the claims differ by argument shape (`breach <deck> <0..1>` and
    `breach <system>`, for instance), so each answers an input the other does not.  A WARNING.
  * **shadowed** -- an earlier claim matches every argument shape a later claim needs, so the later
    claim can never run.  The request it was written to answer is served by the wrong branch.  A
    FAILURE.

The walk sees argument *shape* -- how many arguments a claim requires (`a[0]`, `b[0]`,
`gi.argc() > first + N`), an upper bound where the guard forbids an argument (`!a[0]`), and the
content tokens a guard adds (`sys >= 0`), which are compared by subset.  It does **not** see dead
logic inside a branch: a guard satisfied only by a value no input carries, or a branch that returns
for every argument it accepts, is invisible to it.  Reachability beyond a duplicate is a person's
call; this walk is the part a script can hold.

  tools/console/verbs.py check                  # both directions + the ladder walk (scripts/api-check.sh)
  tools/console/verbs.py derive                 # the console's verb set, one per line
  tools/console/verbs.py enumerate              # verb<TAB>args<TAB>stations, one per line
  tools/console/verbs.py ladder                 # verb<TAB>line<TAB>shape, in dispatch order
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

# Argument-shape terms inside a ladder condition. `_ARG_A`/`_ARG_B` require the argument (a lower
# bound); `_NOT_A`/`_NOT_B` forbid it (an upper bound); `_ARGC_FIRST` requires a count.
_ARG_A = re.compile(r"(?<![!\w])a\s*\[\s*0\s*\]")
_ARG_B = re.compile(r"(?<![!\w])b\s*\[\s*0\s*\]")
_NOT_A = re.compile(r"!\s*a\s*\[\s*0\s*\]")
_NOT_B = re.compile(r"!\s*b\s*\[\s*0\s*\]")
_ARGC_FIRST = re.compile(r"gi\.argc\(\)\s*>\s*first\s*\+\s*(\d+)")
_WORD = re.compile(r"[A-Za-z_]")
_UNBOUNDED = 1 << 30


def _mask(text):
    """`text` with comment bodies and string/char literal contents blanked, length preserved.

    Structural scanning (braces, `if`, `return`) then cannot be fooled by a `{` in a comment or a
    `}` inside a string, while a position in the mask is still a position in `text`.
    """
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
        elif c == "/" and i + 1 < n and text[i + 1] == "*":
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif c in "\"'":
            quote, j = c, i + 1
            while j < n and text[j] != quote:
                j += 2 if text[j] == "\\" else 1
            j = min(j + 1, n)
            for k in range(i + 1, j - 1):
                out[k] = " "
            i = j
        else:
            i += 1
    return "".join(out)


def _matching(s, i):
    """Index of the `)` or `}` that closes the `(`/`{` at `s[i]`, by nesting (same bracket kind)."""
    open_ch = s[i]
    close_ch = ")" if open_ch == "(" else "}"
    depth = 0
    while i < len(s):
        if s[i] == open_ch:
            depth += 1
        elif s[i] == close_ch:
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise ValueError("unbalanced %r" % open_ch)


def _function_body(text, signature):
    """Return (masked text, index of the function's opening `{`, index of its closing `}`).

    The body is matched by brace nesting from the first `{` after `signature`, on the masked text so
    a brace in a comment or a string cannot close it early.
    """
    masked = _mask(text)
    start = masked.index(signature)
    open_brace = masked.index("{", start)
    return masked, open_brace, _matching(masked, open_brace)


def _top_level_only(seg):
    """`seg` with everything inside nested braces removed, so a `return` in a nested block is not
    mistaken for one the enclosing branch always reaches."""
    out, depth = [], 0
    for ch in seg:
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth = max(0, depth - 1)
        elif depth == 0:
            out.append(ch)
    return "".join(out)


class Claim(object):
    """One `Q_stricmp( cmd, "verb" )` in the dispatch ladder, with its argument shape."""

    def __init__(self, verb, line, chain, cond, min_args, max_args, content, terminal):
        self.verb = verb
        self.line = line
        self.chain = chain
        self.cond = cond
        self.min_args = min_args
        self.max_args = max_args
        self.content = content
        self.terminal = terminal

    @property
    def shape(self):
        if self.max_args == 0:
            where = "takes no arguments"
        elif self.min_args == 0 and self.max_args >= _UNBOUNDED:
            where = "takes any arguments"
        elif self.min_args == 0:
            where = "takes at most %d argument(s)" % self.max_args
        else:
            where = "needs at least %d argument(s)" % self.min_args
        if self.content:
            where += " and " + " ".join(sorted(self.content))
        return where

    def __repr__(self):
        return "Claim(%r, line=%d)" % (self.verb, self.line)


def _guard(cond):
    """The argument shape a claim's condition imposes: (min args, max args, content tokens)."""
    lo, hi = 0, _UNBOUNDED
    if _ARG_A.search(cond):
        lo = max(lo, 1)
    if _ARG_B.search(cond):
        lo = max(lo, 2)
    for k in _ARGC_FIRST.findall(cond):
        lo = max(lo, int(k))
    if _NOT_A.search(cond):
        hi = min(hi, 0)
    if _NOT_B.search(cond):
        hi = min(hi, 1)
    rest = _CMD_LITERAL.sub("", cond)
    rest = _ARGC_FIRST.sub("", rest)
    rest = _NOT_A.sub("", rest)
    rest = _NOT_B.sub("", rest)
    rest = _ARG_A.sub("", rest)
    rest = _ARG_B.sub("", rest)
    content = frozenset(tok for tok in re.split(r"[&|!()\s]+", rest) if tok)
    return lo, hi, content


def claims(path):
    """The dispatch ladder's claims, in source order: `Svcmd_Ship_f`'s top-level `if`/`else if`
    statements whose condition tests `cmd` against a verb literal."""
    text = Path(path).read_text(encoding="utf-8")
    masked, open_brace, close_brace = _function_body(text, _FUNC)
    body = masked[open_brace : close_brace + 1]
    found = []
    i, depth, chain = 0, 0, 0
    while i < len(body):
        ch = body[i]
        if ch == "{":
            depth += 1
            i += 1
            continue
        if ch == "}":
            depth -= 1
            i += 1
            continue
        if (
            depth == 1
            and body.startswith("if", i)
            and (i + 2 >= len(body) or not _WORD.match(body[i + 2]))
            and (i == 0 or not _WORD.match(body[i - 1]))
        ):
            back = i - 1
            while back >= 0 and body[back] in " \t\r\n":
                back -= 1
            if not (back >= 3 and body[back - 3 : back + 1] == "else"):
                chain += 1
            paren = body.index("(", i)
            close = _matching(body, paren)
            cond = text[open_brace + paren + 1 : open_brace + close]
            verbs = _CMD_LITERAL.findall(cond)
            if verbs:
                line = text[: open_brace + i].count("\n") + 1
                # The branch body: a braced block, or a single statement up to its `;`.
                j = close + 1
                while j < len(body) and body[j] in " \t\r\n":
                    j += 1
                if j < len(body) and body[j] == "{":
                    end = _matching(body, j)
                    seg = body[j + 1 : end]
                else:
                    end = body.find(";", j)
                    end = len(body) if end < 0 else end
                    seg = body[j:end]
                terminal = bool(re.search(r"\b(return|goto)\b", _top_level_only(seg)))
                lo, hi, content = _guard(cond)
                distinct = []
                for verb in verbs:
                    if verb not in distinct:
                        distinct.append(verb)
                for verb in distinct:
                    found.append(Claim(verb, line, chain, " ".join(cond.split()), lo, hi, content, terminal))
        i += 1
    return found


def derive(path):
    """The console's verb set: every first-token command literal the ladder dispatches on."""
    return sorted({c.verb for c in claims(path)})


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


def _shadows(earlier, later):
    """Can `later` never run for the sake of `earlier`?

    Only if `earlier` matches every argument `later` accepts -- its arity interval contains
    `later`'s and its content constraints are a subset -- **and** `earlier` would be reached first:
    the two are in one `if`/`else if` chain (first match wins), or `earlier`'s branch returns or
    jumps, so the ladder cannot fall through to `later`.
    """
    if earlier.min_args > later.min_args or earlier.max_args < later.max_args:
        return False
    if not earlier.content <= later.content:
        return False
    return earlier.chain == later.chain or earlier.terminal


def collisions(claims_list):
    """Every verb claimed more than once, as (verb, kind, earlier, later) findings in ladder order.

    `kind` is "shadow" (the later claim can never run -- a failure) or "reachable" (the two differ
    by argument shape -- a warning).
    """
    by_verb = {}
    for c in claims_list:
        by_verb.setdefault(c.verb, []).append(c)
    findings = []
    for verb in sorted(by_verb):
        cs = by_verb[verb]
        if len(cs) < 2:
            continue
        shadowed = set()
        for i, earlier in enumerate(cs):
            for later in cs[i + 1 :]:
                if _shadows(earlier, later):
                    findings.append((verb, "shadow", earlier, later))
                    shadowed.add(id(later))
        for i, earlier in enumerate(cs):
            for later in cs[i + 1 :]:
                if id(later) not in shadowed:
                    findings.append((verb, "reachable", earlier, later))
    return findings


def check(console, register, out):
    ladder = claims(console)
    console_verbs = sorted({c.verb for c in ladder})
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

    for verb, kind, earlier, later in collisions(ladder):
        if kind == "shadow":
            failed = True
            out("FAIL  the verb '%s' is claimed twice, and the claim at line %d can never run: "
                "line %d matches every argument shape it needs" % (verb, later.line, earlier.line))
            out("        line %d: %s  (%s)" % (earlier.line, earlier.cond, earlier.shape))
            out("        line %d: %s  (%s) -- shadowed" % (later.line, later.cond, later.shape))
        else:
            out("WARN  the verb '%s' is claimed twice and both can be reached (they differ by argument shape)" % verb)
            out("        line %d: %s  (%s)" % (earlier.line, earlier.cond, earlier.shape))
            out("        line %d: %s  (%s)" % (later.line, later.cond, later.shape))

    if failed:
        return 1
    out("PASS  every console verb is in the ship's API, and every API verb is answered by the console")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=["check", "derive", "enumerate", "ladder"])
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
    if args.mode == "ladder":
        for c in claims(args.console):
            print("%s\tline %d\t%s" % (c.verb, c.line, c.shape))
        return 0
    try:
        return check(args.console, args.register, lambda s: print(s))
    except ValueError as e:
        print("FAIL  %s" % e)
        return 1


if __name__ == "__main__":
    sys.exit(main())
