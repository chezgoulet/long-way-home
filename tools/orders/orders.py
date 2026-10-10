#!/usr/bin/env python3
"""The order set's freshness check (docs/orders.md, Task B).

The common set of orders -- the enumerated options a one-recipient order brief offers -- lives in
one place a program can read: `module/ship/order_set.def` (included by `module/ship/ship_core.h` to
declare the `OrderClass` enum and to build the spec table). This tool checks that register against
the surface that keeps growing, the dispatch (`Ship::ApplyOrder` in `module/ship/ship_core.cpp`), in
**both** directions:

  * an order class in the register that the dispatch does not handle  -> FAIL (the omission drift)
  * a `case ORDER_...` the dispatch handles and the register lacks    -> FAIL (the foreign row)

The pattern is `tools/console/verbs.py` + `scripts/api-check.sh`: the register is curated, the
surface is derived from the code, and a check fails when they drift in either direction. This tool
does not judge *who* may be given an order -- that is the derivation's (OrderPermitted/DeriveOrders)
and the run-time guard's (OrderSetFault) job; it checks that every class the register names is a
class the simulation can actually apply.

  tools/orders/orders.py check                  # both directions (what scripts/order-check.sh runs)
  tools/orders/orders.py enumerate              # token<TAB>needs<TAB>job, one per line
  tools/orders/orders.py check --register X --dispatch Y   # check modified copies, tree untouched
"""

import argparse
import re
import sys
from pathlib import Path

# The dispatch: the one function that turns an order class into ship state, case by case.
_DISPATCH_FUNC = "void ApplyOrder(Ship &s, uint8_t order, int16_t target)"
_CASE = re.compile(r'case\s+(ORDER_[A-Z0-9_]+)\s*:')
_ROW = re.compile(
    r'^\s*ORDER_CLASS\(\s*"?(?P<token>[A-Z_][A-Z0-9_]*)"?\s*,\s*'
    r'"[^"]*"\s*,\s*"[^"]*"\s*,\s*"[^"]*"\s*,\s*'
    r'(?P<needs>[A-Z_| ]+?)\s*,\s*'
    r'(?P<job>JOB_[A-Z0-9_]+)\s*,\s*'
    r'(?P<target>-?\d+)\s*\)',
    re.M,
)

# The needs and job tokens the register may name. A typo is a check failure, not a silent skip.
_NEEDS = {"ORD_NEED_POST", "ORD_NEED_COMMAND", "ORD_NEED_ENGINEERING", "ORD_NEED_SECURITY",
          "ORD_NEED_MEDICAL", "ORD_NEED_SCIENCE", "ORD_NEED_JUNIOR"}
_JOBS = {"JOB_REPAIR", "JOB_SEAL", "JOB_RECLAIM", "JOB_BUILD", "JOB_SECURE", "JOB_EVACUATE"}


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
                return text[open_brace:i + 1]
    raise ValueError("unbalanced braces after %r" % signature)


def derive_dispatch(path):
    """The order classes the dispatch handles: every `case ORDER_...` in ApplyOrder."""
    body = _function_body(Path(path).read_text(encoding="utf-8"), _DISPATCH_FUNC)
    return sorted(set(_CASE.findall(body)))


def enumerate_register(path):
    """The register's rows as (token, needs, job, target); raises on a malformed row."""
    text = Path(path).read_text(encoding="utf-8")
    rows = _ROW.findall(text)
    declared = len(re.findall(r"^\s*ORDER_CLASS\(", text, re.M))
    if declared != len(rows):
        raise ValueError("%d ORDER_CLASS line(s) but only %d parsed in %s" % (declared, len(rows), path))
    seen = set()
    out = []
    for token, needs, job, target in rows:
        if token in seen:
            raise ValueError("duplicate order class %r" % token)
        seen.add(token)
        bits = [b.strip() for b in needs.split("|") if b.strip()]
        if not bits:
            raise ValueError("order %r names no need" % token)
        for b in bits:
            if b not in _NEEDS:
                raise ValueError("order %r names an unknown need %r (allowed: %s)"
                                 % (token, b, ", ".join(sorted(_NEEDS))))
        if job not in _JOBS:
            raise ValueError("order %r names an unknown job kind %r (allowed: %s)"
                             % (token, job, ", ".join(sorted(_JOBS))))
        out.append((token, needs.strip(), job, int(target)))
    return out


def check(register, dispatch, out):
    rows = enumerate_register(register)
    registered = sorted({r[0] for r in rows})
    handled = derive_dispatch(dispatch)

    out("orders.py: %s enumerates %d order class(es); %s handles %d"
        % (register, len(registered), dispatch, len(handled)))

    missing = [t for t in registered if t not in handled]
    extra = [t for t in handled if t not in registered]

    failed = False
    if missing:
        failed = True
        out("FAIL  the register names and the dispatch does not handle (%d): %s" % (len(missing), " ".join(missing)))
    if extra:
        failed = True
        out("FAIL  the dispatch handles and the register does not name (%d): %s" % (len(extra), " ".join(extra)))
    if failed:
        return 1
    out("PASS  every order class in the register is handled by the dispatch, and every case is registered")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode", choices=["check", "enumerate"])
    ap.add_argument("--register", default="module/ship/order_set.def")
    ap.add_argument("--dispatch", default="module/ship/ship_core.cpp")
    args = ap.parse_args()

    if args.mode == "enumerate":
        for token, needs, job, target in enumerate_register(args.register):
            print("%s\t%s\t%s\t%d" % (token, needs, job, target))
        return 0
    try:
        return check(args.register, args.dispatch, lambda s: print(s))
    except ValueError as e:
        print("FAIL  %s" % e)
        return 1


if __name__ == "__main__":
    sys.exit(main())
