#!/usr/bin/env python3
"""Derive a reachability report for the ship public surface (docs/hook-register.md, Task B).

This is the *derivation* half of the register, not its check.  It answers one question per hook:
from which of the game's entry surfaces can a call to it start?  A person turns that into the
register's three states; the tool deliberately does not, because "is this a readout a console
draws" and "does a screen send this command" are judgements about art and UI, not about C++.

The method is a transitive closure, not a direct-call count, because a hook that only the
developer console reaches must not be reported as reached in play just because a play function
calls it -- that is the false green the register exists to prevent (``OfferRising`` is the worked
example: ``ship rise`` calls it, and ``OfferRisingTo`` calls it, but no play surface does).

Entry surfaces, each derived from the code:

  ui        a command an in-game station screen sends (module/ui -> `ship as <st> <cmd>`); the
            command's branch of Svcmd_Ship_f is the entry
  publish   Publish() populates the cvars the consoles draw: the hook's reading is on a screen
  world     module/crew, module/cgame, lwh_panel, lwh_entities: the crew, environment and player
            layers
  advance   the simulation's own Advance(): a situation it generates on its own
  console   Svcmd_Ship_f: the developer console (`ship ...`)
  harness   RunTest / g_shipTest: a developer test harness

``play`` is the closure from ui+publish+world+advance; ``dev`` from console+harness; ``test``
from tests/.  ``-`` means nothing in this repository calls it.
"""

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from surface import public_functions, strip_comments  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
CORE = ROOT / "module/ship/ship_core.cpp"
GSHIP = ROOT / "module/ship/g_ship.cpp"

# Functions reachable from a key a player presses on an in-game station screen, by the `ship ...`
# command the screen sends (module/ui/*.cpp) and the branch of Svcmd_Ship_f that command takes.
UI_FUNCTIONS = {
    "SetEnabled", "SetPriority", "SetAllocation", "SetPowerAuto", "PowerAuto",
    "AcceptRecommendation", "RefuseRecommendation", "GrantBand", "RevokeBand",
    "MakeBreach", "BreachScore", "CounterHack",
    "SetAlert",
    "FireTorpedo", "SetPhaserYield", "PhaserYieldName", "PhaserYieldOf",
    "TransporterConditionLine", "TransportAway", "TransportBack", "AwayTeam",
    "Hail", "Trade", "AnswerDistress", "SetForceField", "SetForceFieldLevel",
    "Jump", "SetCourse", "PlotCourse", "Disengage",
    "Scan", "RevealPhenomenon", "ScanCompartment",
    "OrderRepairFirst", "OrderSecurityTo", "OrderEvacuate", "OrderTriage",
    "BeginOverride", "ConfirmOverride", "Promote", "WriteOff", "OrderBuild", "SetJobPriority",
    "CreateCharacter",
    "SetSurgicalField", "SurgicalField", "ActivateEMH", "EMHActive", "RecoverCaptive",
    "DraftReport", "OpenReport", "StrikeReportLine", "SoftenReportLine", "EditReportLine",
    "AddReportLine", "SignReport", "ReportAudienceName", "Reports", "ReportDiff", "PurgeLogs",
}

WORLD_GLOBS = [
    "module/crew/*.cpp",
    "module/cgame/*.cpp",
    "module/ship/lwh_panel.cpp",
    "module/ship/lwh_entities.cpp",
]

_DEF = re.compile(r"(?m)^[ \t]*(?:[A-Za-z_][\w:<>]*[\s*&]+)+([A-Za-z_]\w*)\s*\(")


def read(path: Path) -> str:
    return strip_comments(path.read_text(encoding="utf-8", errors="replace"))


def slice_between(text: str, start: str, end: str | None) -> str:
    i = text.find(start)
    if i < 0:
        return ""
    j = text.find(end) if end else -1
    return text[i:j] if j >= 0 else text[i:]


def called_names(text: str, universe: set) -> set:
    got = set()
    for m in re.finditer(r"\b([A-Za-z_]\w*)\s*\(", text):
        if m.group(1) in universe:
            got.add(m.group(1))
    return got


def core_bodies(core: str, universe: set):
    """Return {function: set(callees)} for definitions in ship_core.cpp."""
    starts = []
    for m in _DEF.finditer(core):
        name = m.group(1)
        if name not in universe:
            continue
        brace = core.find("{", m.end())
        if brace < 0:
            continue
        # reject a match whose body opens far away (a declaration, not a definition)
        if ";" in core[m.end():brace]:
            continue
        depth, i = 0, brace
        while i < len(core):
            if core[i] == "{":
                depth += 1
            elif core[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        starts.append((m.start(), i, name, core[brace:i]))
    edges = {name: called_names(body, universe) for _, _, name, body in starts}
    return edges


def closure(entries: set, edges: dict) -> set:
    seen = set(entries)
    stack = list(entries)
    while stack:
        f = stack.pop()
        for g in edges.get(f, ()):  # noqa: E113
            if g not in seen:
                seen.add(g)
                stack.append(g)
    return seen


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("header", nargs="?", default=str(ROOT / "module/ship/ship_core.h"))
    ap.add_argument("--detail", action="store_true", help="also print the direct call-site tags")
    args = ap.parse_args()

    names = public_functions(Path(args.header).read_text(encoding="utf-8"))
    core = read(CORE)
    gship = read(GSHIP)
    # The graph must include the model's own `static` helpers, not only the public surface: a play
    # entry point reaches a hook through them, and stopping at a helper would report the hook as
    # console-only (the false negative this report exists to avoid).
    inner = {m.group(1) for m in _DEF.finditer(core)}
    universe = inner | set(names)
    edges = core_bodies(core, universe)

    publish = slice_between(gship, "void Publish( void )", "void ReportWatchedTrigger")
    console = slice_between(gship, "void Svcmd_Ship_f( void )", None)
    harness = slice_between(gship, "void RunTest( void )", "static void BorgAssets")
    advance = slice_between(core, "static void Advance(Ship &s, double shipSecondsTotal)", "const char *DilithiumWayName")
    world = "\n".join(read(p) for g in WORLD_GLOBS for p in sorted(ROOT.glob(g)))
    tests = "\n".join(read(p) for p in sorted(ROOT.glob("tests/**/*.cpp")))

    entries = {
        "ui": set(UI_FUNCTIONS),
        "publish": called_names(publish, universe),
        "world": called_names(world, universe),
        "advance": called_names(advance, universe),
        "console": called_names(console, universe),
        "harness": called_names(harness, universe),
        "test": called_names(tests, universe),
    }
    play = closure(entries["ui"] | entries["publish"] | entries["world"] | entries["advance"], edges)
    dev = closure(entries["console"] | entries["harness"], edges) - play
    test = closure(entries["test"], edges) - play - dev

    print("hook\tplay\tdev\ttest" + ("\tvia" if args.detail else ""))
    for n in names:
        if n in play:
            state, via = "play", "play"
        elif n in dev:
            state, via = "dev", "console"
        elif n in test:
            state, via = "test", "tests"
        else:
            state, via = "-", "-"
        line = "\t".join([n, "1" if state == "play" else "0",
                          "1" if state == "dev" else "0",
                          "1" if state == "test" else "0"])
        if args.detail:
            tags = [k for k in ("ui", "publish", "world", "advance", "console", "harness", "test")
                    if n in entries[k]]
            line += "\t" + ",".join(tags)
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
