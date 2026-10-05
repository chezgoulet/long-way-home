#!/usr/bin/env python3
"""Stitch per-deck map sources into one whole-ship map (gate S3).

    stitch.py --decks DIR --out ship.map [--pitch 3072] [--report report.json]

DIR holds deckNN.map sources (the published Virtual Voyager decks). Each deck is authored in its
own map at the same place; this puts each at its own height and merges them:

  * the single stray brush every source carries near the origin is dropped -- it is what makes a
    slab 500 units thick look 4,000 units tall
  * every deck is moved down by (deck - 1) * pitch. The pitch is a multiple of 1024 so that
    textures, which are aligned in world space, land where they were
  * `func_group` is dissolved into the world (it is an editor grouping, not an entity)
  * `func_static` and `func_wall` that nothing can address (no targetname) become world geometry
  * names that entities on more than one deck define are prefixed with their deck (d04_door1), in
    every key that carries a name, so one deck's button cannot open another deck's door. Names
    defined on one deck only, and names the maps only refer to (the player's), are left alone
  * one player start is kept (the first deck's); the others become named arrival points

The report says what was done and what is left: brush models by class against the engine's limit,
and the renamed names -- any ICARUS script that addresses one of those by its old name will not
find it, and that list is the work still owed.

Nothing here reads game data; the sources are the publisher's own released map files.
"""

import argparse
import collections
import glob
import json
import os
import re
import sys

POINT = re.compile(r"\(\s*(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?)\s+(-?\d+(?:\.\d+)?)\s*\)")
PATCH_ROW = re.compile(r"\(\s*(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s+(-?[\d.]+)\s*\)")
KV = re.compile(r'^"([^"]*)"\s+"([^"]*)"$')

# Keys whose value is the name of another entity (or this one).
# Keys that give an entity its own name. A name is only renamed if entities on more than one deck
# *define* it; a name that maps merely refer to (the player's "munro", say) belongs to something the
# maps do not create, and must be left alone.
DEFINING_KEYS = ("targetname", "NPC_targetname", "npc_targetname", "script_targetname", "team")
NAME_KEYS = ("targetname", "target", "target2", "target3", "target4", "killtarget", "paintarget", "opentarget",
             "closetarget", "NPC_targetname", "npc_targetname", "NPC_target", "npc_target", "script_targetname",
             "team", "ownername", "cameraGroup", "enemy", "goaltarget", "falsetarget")
FOLDABLE = ("func_static", "func_wall")
STRAY_Z = -2048       # deck geometry lies well below this; the stray brush sits near zero
MODEL_LIMIT = 256     # the engine's model index, before S3's patch


def fmt(v):
    return ("%.4f" % v).rstrip("0").rstrip(".") if v != int(v) else str(int(v))


class Brush:
    def __init__(self):
        self.lines = []
        self.zmin, self.zmax = 1e9, -1e9
        self.is_patch = False

    def add(self, line):
        self.lines.append(line)
        if line.startswith("patchDef"):
            self.is_patch = True
        # In a patch only the control rows ("( ( x y z s t ) ... )") are positions; the header
        # "( width height 0 0 0 )" has the same shape and must be neither measured nor moved.
        if self.is_patch:
            found = PATCH_ROW.finditer(line) if line.startswith("( (") else ()
        else:
            found = POINT.finditer(line) if line.startswith("(") else ()
        for m in found:
            z = float(m.group(3))
            self.zmin, self.zmax = min(self.zmin, z), max(self.zmax, z)

    def moved(self, dz):
        if not dz:
            return list(self.lines)
        out = []
        for line in self.lines:
            if self.is_patch:
                if not line.startswith("( ("):
                    out.append(line)
                    continue
                line = PATCH_ROW.sub(lambda m: "( %s %s %s %s %s )" % (m.group(1), m.group(2), fmt(float(m.group(3)) + dz),
                                                                       m.group(4), m.group(5)), line)
            elif line.startswith("("):
                # only the three plane points; the texture's shift/rotate/scale that follow are left alone
                head = POINT.sub(lambda m: "( %s %s %s )" % (m.group(1), m.group(2), fmt(float(m.group(3)) + dz)), line, count=3)
                line = head
            out.append(line)
        return out


def parse(path):
    """A .map as a list of (keys, brushes). Keys keep their order."""
    ents, depth, keys, brushes, cur = [], 0, None, None, None
    with open(path, encoding="latin-1") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("//"):
                continue
            if line == "{":
                depth += 1
                if depth == 1:
                    keys, brushes = collections.OrderedDict(), []
                elif depth == 2:
                    cur = Brush()
                else:
                    cur.add(line)
            elif line == "}":
                if depth == 1:
                    ents.append((keys, brushes))
                elif depth == 2:
                    brushes.append(cur)
                    cur = None
                else:
                    cur.add(line)
                depth -= 1
            elif depth == 1:
                m = KV.match(line)
                if m:
                    keys[m.group(1)] = m.group(2)
            elif depth >= 2:
                cur.add(line)
    if depth != 0:
        raise ValueError(f"{path}: unbalanced braces")
    if not ents or ents[0][0].get("classname") != "worldspawn":
        raise ValueError(f"{path}: the first entity is not worldspawn")
    return ents


def shift_origin(value, dz):
    try:
        x, y, z = (float(v) for v in value.split())
    except ValueError:
        return value
    return "%s %s %s" % (fmt(x), fmt(y), fmt(z + dz))


def stitch(deck_files, pitch):
    """deck_files: {deck number: path}. Returns (world_keys, world_brush_lines, entities, report)."""
    if pitch % 1024:
        raise ValueError("pitch must be a multiple of 1024, or world-aligned textures will slip")
    decks = {n: parse(p) for n, p in sorted(deck_files.items())}

    # Which names collide across decks.
    seen = collections.defaultdict(set)
    for n, ents in decks.items():
        for keys, _ in ents:
            for k in DEFINING_KEYS:
                if keys.get(k):
                    seen[keys[k].lower()].add(n)
    shared = {name for name, where in seen.items() if len(where) > 1}

    first = min(decks)
    world_keys = collections.OrderedDict(decks[first][0][0])
    world, out = [], []
    report = {"decks": {}, "pitch": pitch, "renamed": sorted(shared), "models": collections.Counter(),
              "dropped_stray_brushes": 0, "folded": collections.Counter()}

    for n, ents in decks.items():
        dz = -(n - 1) * pitch
        tag = "d%02d_" % n
        zs = []
        for i, (keys, brushes) in enumerate(ents):
            cls = keys.get("classname", "")
            keep = []
            for b in brushes:
                if b.zmin > STRAY_Z:
                    report["dropped_stray_brushes"] += 1
                    continue
                keep.append(b)
                zs += [b.zmin + dz, b.zmax + dz]
            if i == 0 or cls == "func_group" or (cls in FOLDABLE and not keys.get("targetname")):
                if i:
                    report["folded"][cls] += 1
                world += [b.moved(dz) for b in keep]
                continue

            keys = collections.OrderedDict(keys)
            for k in NAME_KEYS:
                if keys.get(k) and keys[k].lower() in shared:
                    keys[k] = tag + keys[k]
            if "origin" in keys:
                keys["origin"] = shift_origin(keys["origin"], dz)
            if cls == "info_player_start" and n != first:
                keys["classname"] = "info_notnull"
                keys["targetname"] = tag + "arrival"
            keys["lwh_deck"] = str(n)
            if keep:
                report["models"][cls] += 1
            out.append((keys, [b.moved(dz) for b in keep]))
        report["decks"][n] = {"entities": len(ents), "z": [min(zs), max(zs)] if zs else None}

    report["world_brushes"] = len(world)
    report["entities"] = len(out) + 1
    report["brush_models"] = sum(report["models"].values())
    report["model_limit"] = MODEL_LIMIT
    report["models"] = dict(report["models"].most_common())
    report["folded"] = dict(report["folded"])

    # No two decks may share space.
    spans = sorted((v["z"][0], v["z"][1], n) for n, v in report["decks"].items() if v["z"])
    report["overlaps"] = [[a[2], b[2]] for a, b in zip(spans, spans[1:]) if b[0] < a[1]]
    return world_keys, world, out, report


def write(path, world_keys, world, ents):
    with open(path, "w", encoding="latin-1") as f:
        def entity(keys, brushes, index):
            f.write("// entity %d\n{\n" % index)
            for k, v in keys.items():
                f.write('"%s" "%s"\n' % (k, v))
            for lines in brushes:
                f.write("{\n" + "\n".join(lines) + "\n}\n")
            f.write("}\n")
        entity(world_keys, world, 0)
        for i, (keys, brushes) in enumerate(ents, 1):
            entity(keys, brushes, i)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--decks", required=True, help="directory holding deckNN.map sources")
    ap.add_argument("--out", required=True)
    ap.add_argument("--pitch", type=int, default=3072, help="vertical distance between decks (multiple of 1024)")
    ap.add_argument("--report")
    a = ap.parse_args(argv)

    files = {}
    for path in glob.glob(os.path.join(a.decks, "**", "deck*.map"), recursive=True):
        m = re.search(r"deck(\d+)\.map$", os.path.basename(path), re.I)
        if m:
            files[int(m.group(1))] = path
    if not files:
        print(f"no deckNN.map under {a.decks}", file=sys.stderr)
        return 2
    try:
        world_keys, world, ents, report = stitch(files, a.pitch)
    except ValueError as e:
        print(str(e), file=sys.stderr)
        return 2
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    write(a.out, world_keys, world, ents)
    if a.report:
        with open(a.report, "w") as f:
            json.dump(report, f, indent=1)

    print(f"{len(files)} decks -> {a.out}")
    print(f"  world brushes {report['world_brushes']}, entities {report['entities']}, "
          f"stray brushes dropped {report['dropped_stray_brushes']}, folded into the world {report['folded']}")
    print(f"  brush models {report['brush_models']} (engine limit {MODEL_LIMIT}): {report['models']}")
    print(f"  names renamed because more than one deck uses them: {len(report['renamed'])}")
    for n, v in sorted(report["decks"].items()):
        print(f"  deck {n:2}: z {v['z'][0]:.0f} .. {v['z'][1]:.0f}")
    if report["overlaps"]:
        print(f"  OVERLAP between decks {report['overlaps']}: raise --pitch", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
