#!/usr/bin/env python3
"""Enumerate the ship's panels, and mark the ones the map's own source did not.

    markpanels.py --map build/ship/voyager.map [--module module/ship/lwh_panel.cpp]
                  [--mark] [--report FILE] [--require-marked]

O10 (`docs/omissions.md`), the in-world control at the panel (`docs/evidence/the-panel-control.md`).

**A panel is the brush whose surface carries the ship's own readout.** The readout shader is the
module's `PANEL_READOUT` (`textures/lwh/panel`, the image `gi.UpdatePanelImage` overwrites); the
surfaces that carry it are the readout brush itself and the published decks' console shaders, which
the module remaps to it. The **list** is the module's own (`module/ship/lwh_panel.cpp`
`PANEL_REMAPPED[]` plus `PANEL_READOUT`): this tool reads it from the module so the enumeration, the
marking and the check cannot disagree with the remap -- change the module's list and all three move.

What the tool reports:

  * **the readout brush** -- the screens the deck tooling authors with `lwh/panel`. These are the
    affordance: `--mark` wraps any still in worldspawn as a named `func_static` (`lwh_panel*`, the
    module's `LWH_PanelTag`), so the world's use can find the brush.
  * **the console shaders** -- the published decks' station consoles, which the module remaps to the
    readout. They are the *read*, and they are **not** marked: converting them to brush entities
    removes sealing geometry from the structural BSP and **the map leaks** (measured; see the
    evidence). The report names them so the omission is not silent.

The panel is identified by the shader (the module's list), never by size, and never as a
`func_usable` (those are doors and hatches -- G20's other half). The tool marks by the *exact* shader
test, not a heuristic.
"""

import argparse
import collections
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "mapgen"))
sys.path.insert(0, HERE)
import stitch  # noqa: E402


def read_panel_list(module_path):
    """The module's own panel shader list: (remapped shaders, readout shader, tag prefix).

    Read from the source so there is exactly one list. A shape this does not recognise is an error,
    not a silent fallback: the check must not pass on a list that was not found.
    """
    text = open(module_path, encoding="utf-8").read()
    m = re.search(r"PANEL_REMAPPED\[\]\s*=\s*\{(.*?)\}\s*;", text, re.S)
    if not m:
        raise ValueError(f"{module_path}: PANEL_REMAPPED[] not found")
    remapped = re.findall(r'"([^"]+)"', m.group(1))
    r = re.search(r'PANEL_READOUT\s*=\s*"([^"]+)"', text)
    if not r:
        raise ValueError(f"{module_path}: PANEL_READOUT not found")
    t = re.search(r'LWH_PanelTag\( void \)\s*\{[^}]*return\s+"([^"]+)"', text, re.S)
    if not t:
        raise ValueError(f"{module_path}: LWH_PanelTag not found")
    if not remapped:
        raise ValueError(f"{module_path}: PANEL_REMAPPED[] is empty")
    return remapped, r.group(1), t.group(1)


def norm(shader):
    """A shader name as a .map face writes it: the module's names carry the `textures/` prefix the
    compiler prepends, and map faces do not (mapgen.py's convention 1). One of each would never
    intersect, so both sides are normalised to the bare form."""
    s = shader.lower()
    return s[len("textures/"):] if s.startswith("textures/") else s


def face_textures(b):
    out = []
    for ln in b.lines:
        if ln.startswith("("):
            m = stitch.FACE_TEXTURE.match(ln)
            if m:
                out.append(norm(m.group(2)))
    return out


def classify(map_path, panel):
    """Every worldspawn brush carrying a panel shader, with its shader set."""
    world_keys, world = stitch.parse(map_path)[0]
    rows = []
    for b in world:
        texs = set(face_textures(b))
        hit = texs & panel
        if hit:
            rows.append({"shaders": sorted(hit), "lo": [int(v) for v in b.lo],
                         "hi": [int(v) for v in b.hi]})
    return rows


def emit(keys, brushes):
    s = "{\n"
    for k, v in keys.items():
        s += '"%s" "%s"\n' % (k, v)
    for b in brushes:
        s += "{\n" + "\n".join(b.lines) + "\n}\n"
    return s + "}\n"


def mark(map_path, out_path, readout, tag):
    """Wrap every worldspawn brush carrying the readout shader as a named func_static.

    Only the readout shader: the console shaders are structural and converting them leaks the map.
    """
    ents = stitch.parse(map_path)
    world_keys, world = ents[0]
    keep, marked = [], []
    for b in world:
        if readout in set(face_textures(b)):
            keys = collections.OrderedDict([("classname", "func_static"),
                                            ("targetname", "%s_%d" % (tag, len(marked) + 1))])
            marked.append(emit(keys, [b]))
        else:
            keep.append(b)
    out = [emit(world_keys, keep)] + marked + [emit(k, br) for k, br in ents[1:]]
    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, "w", encoding="latin-1") as f:
        f.write("".join(out))
    return len(marked)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--map", required=True)
    ap.add_argument("--module", default=os.path.join(HERE, "..", "..", "module", "ship", "lwh_panel.cpp"))
    ap.add_argument("--mark", action="store_true", help="wrap readout brushes still in worldspawn as lwh_panel* entities")
    ap.add_argument("--out", help="where --mark writes (default: the map, in place)")
    ap.add_argument("--report")
    ap.add_argument("--require-marked", action="store_true",
                    help="exit non-zero if a worldspawn brush still carries the readout shader")
    a = ap.parse_args(argv)

    raw_remapped, raw_readout, tag = read_panel_list(a.module)
    remapped, readout = [norm(s) for s in raw_remapped], norm(raw_readout)
    panel = set(remapped) | {readout}
    rows = classify(a.map, panel)

    # The readout brush is the affordance; the console shaders are the read (they cannot be marked).
    read_rows = [r for r in rows if readout in r["shaders"]]
    console_rows = [r for r in rows if readout not in r["shaders"]]

    if a.mark and read_rows:
        n = mark(a.map, a.out or a.map, readout, tag)
        print("marked %d readout brush(es) as %s*" % (n, tag))
        rows = classify(a.map, panel)
        read_rows = [r for r in rows if readout in r["shaders"]]
        console_rows = [r for r in rows if readout not in r["shaders"]]

    report = {"module_list": {"remapped": raw_remapped, "readout": raw_readout, "tag": tag},
              "readout_brushes_world": len(read_rows),
              "console_brushes_world": len(console_rows),
              "console_shaders": sorted({s for r in console_rows for s in r["shaders"]})}
    if a.report:
        with open(a.report, "w") as f:
            json.dump(report, f, indent=1)

    print("panel list (from %s): %s + %s" % (os.path.basename(a.module),
          ", ".join(raw_remapped), raw_readout))
    print("  readout brushes still in worldspawn: %d" % len(read_rows))
    print("  console-shader brushes (the read, not marked): %d spanning %s"
          % (len(console_rows), ", ".join(sorted({s for r in console_rows for s in r["shaders"]})) or "none"))

    if a.require_marked and read_rows:
        print("FAIL  %d readout brush(es) are still world geometry: the affordance is not placed"
              % len(read_rows), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
