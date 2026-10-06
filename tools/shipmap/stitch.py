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
  * a trigger made of one axis-aligned box brush becomes an origin with mins and maxs -- a trigger
    needs a volume, not geometry -- and stops costing a brush model (needs patches/0011 in the game)
  * one player start is kept (the first deck's); every deck gets a named arrival point (d04_arrival)
  * a `target_level_change` to another deck of the ship becomes a `target_teleporter` to that deck's
    arrival point: the turbolift now travels within the map. Level changes to anywhere else (the
    brig, the holodeck programs, the campaign) are left as they were

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
# Textures the published sources name that the shipped game does not have, and the nearest thing it
# does. Chosen by name, not by eye: each is a guess to be looked at in the ship and corrected here.
SUBSTITUTE = {
    "hall/hallcomp": "hall/hallcomp2",
    "hall/hallfloor2": "hall/hallfloor1",
    "hall/supportsegment_side3": "hall/supportsegment_side2",
    "voyager/runnerlightsra": "voyager/runnerlights",
}
FACE_TEXTURE = re.compile(r"^(\((?:[^()]*\)\s*\(){2}[^()]*\)\s+)(\S+)")
STRAY_Z = -2048       # deck geometry lies well below this; the stray brush sits near zero
MODEL_LIMIT = 256     # the engine's model index, before S3's patch


# NB: station markers on the published decks. Each system's station is on a fixed deck (ship_core's
# SystemSpec), and g_crew.cpp's StationFor looks for a map entity "lwh_station_<system>". Placing one
# at the published maps' own interface panels was tried and reverted: those panels use sky/trigger
# shaders, so a marker at a panel's centre sits in the void, q3map2 reports "Entity leaked", writes no
# portal file, and -vis fails. The markers need a chosen open-space origin on the right deck -- the S5
# brief's "record the chosen origins as invention" -- which needs a person who can see the map.


def fmt(v):
    return ("%.4f" % v).rstrip("0").rstrip(".") if v != int(v) else str(int(v))


class Brush:
    def __init__(self):
        self.lines = []
        self.zmin, self.zmax = 1e9, -1e9
        self.lo, self.hi = [1e9] * 3, [-1e9] * 3
        self.faces, self.axial = 0, True
        self.is_patch = False

    substituted = 0

    def add(self, line):
        # the texture: second token-group of a brush face, or a patch's line of its own
        if self.is_patch and line.lower() in SUBSTITUTE:
            line = SUBSTITUTE[line.lower()]
            Brush.substituted += 1
        elif line.startswith("(") and not self.is_patch:
            m = FACE_TEXTURE.match(line)
            if m and m.group(2).lower() in SUBSTITUTE:
                line = m.group(1) + SUBSTITUTE[m.group(2).lower()] + line[m.end():]
                Brush.substituted += 1
        self.lines.append(line)
        if line.startswith("patchDef"):
            self.is_patch = True
        # In a patch only the control rows ("( ( x y z s t ) ... )") are positions; the header
        # "( width height 0 0 0 )" has the same shape and must be neither measured nor moved.
        if self.is_patch:
            found = PATCH_ROW.finditer(line) if line.startswith("( (") else ()
        else:
            found = POINT.finditer(line) if line.startswith("(") else ()
        pts = []
        for m in found:
            p = (float(m.group(1)), float(m.group(2)), float(m.group(3)))
            pts.append(p)
            for i in range(3):
                self.lo[i], self.hi[i] = min(self.lo[i], p[i]), max(self.hi[i], p[i])
            z = p[2]
            self.zmin, self.zmax = min(self.zmin, z), max(self.zmax, z)
        if not self.is_patch and len(pts) >= 3:
            self.faces += 1
            # an axis-aligned face has all three of its points on one coordinate plane
            if not any(pts[0][i] == pts[1][i] == pts[2][i] for i in range(3)):
                self.axial = False

    def is_box(self):
        return not self.is_patch and self.faces == 6 and self.axial

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


def is_interface_name(keys, key):
    """A target_interface's script_targetname is not the name of an entity: it is the name of the
    screen the panel opens ("turbolift", "transporter", "log7"). Renaming it leaves a panel that
    opens nothing."""
    return key == "script_targetname" and keys.get("classname") == "target_interface"


def shift_origin(value, dz):
    try:
        x, y, z = (float(v) for v in value.split())
    except ValueError:
        return value
    return "%s %s %s" % (fmt(x), fmt(y), fmt(z + dz))


def stitch(deck_files, pitch):
    """deck_files: {deck number: path}. Returns (world_keys, world_brush_lines, entities, report)."""
    Brush.substituted = 0
    if pitch % 1024:
        raise ValueError("pitch must be a multiple of 1024, or world-aligned textures will slip")
    decks = {n: parse(p) for n, p in sorted(deck_files.items())}

    # Which names collide across decks.
    seen = collections.defaultdict(set)
    for n, ents in decks.items():
        for keys, _ in ents:
            for k in DEFINING_KEYS:
                if keys.get(k) and not is_interface_name(keys, k):
                    seen[keys[k].lower()].add(n)
    shared = {name for name, where in seen.items() if len(where) > 1}

    first = min(decks)
    world_keys = collections.OrderedDict(decks[first][0][0])
    world, out, late = [], [], []
    report = {"decks": {}, "pitch": pitch, "renamed": sorted(shared), "models": collections.Counter(), "station_markers": 0,
              "dropped_stray_brushes": 0, "folded": collections.Counter(), "turbolift_links": 0, "turbolift_links_added": 0, "triggers_boxed": 0,
              "level_changes_left": []}

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
                if keys.get(k) and keys[k].lower() in shared and not is_interface_name(keys, k):
                    keys[k] = tag + keys[k]
            if "origin" in keys:
                keys["origin"] = shift_origin(keys["origin"], dz)
            if cls == "info_player_start":
                # Where the turbolift sets you down on this deck. The first deck's start is also
                # where the game begins, so it is kept as a start and given an arrival point too.
                if n == first:
                    arrival = collections.OrderedDict(keys)
                    arrival["classname"], arrival["targetname"], arrival["lwh_deck"] = "info_notnull", tag + "arrival", str(n)
                    out.append((arrival, []))
                else:
                    keys["classname"] = "info_notnull"
                    keys["targetname"] = tag + "arrival"
            if cls == "target_level_change":
                # The turbolift asked for another level. If that level is a deck of this ship, it is
                # now a place in the same map: go there instead.
                m = re.match(r"tour/deck(\d+)$", keys.get("mapname", ""), re.I)
                if m and int(m.group(1)) in decks:
                    keys["classname"] = "target_teleporter"
                    keys["target"] = "d%02d_arrival" % int(m.group(1))
                    del keys["mapname"]
                    report["turbolift_links"] += 1
                else:
                    report["level_changes_left"].append(keys.get("mapname", "?"))
            keys["lwh_deck"] = str(n)
            if cls.startswith("trigger_") and len(keep) == 1 and keep[0].is_box() and "origin" not in keys:
                # A trigger needs a volume, not geometry: a single box brush becomes an origin with
                # mins and maxs, and the map is one brush model lighter (the game takes either form).
                b = keep[0]
                centre = [(b.lo[i] + b.hi[i]) / 2 for i in range(3)]
                half = [(b.hi[i] - b.lo[i]) / 2 for i in range(3)]
                keys["origin"] = "%s %s %s" % (fmt(centre[0]), fmt(centre[1]), fmt(centre[2] + dz))
                keys["mins"] = "%s %s %s" % tuple(fmt(-h) for h in half)
                keys["maxs"] = "%s %s %s" % tuple(fmt(h) for h in half)
                report["triggers_boxed"] += 1
                late.append((keys, []))   # not compiled: see write()
                continue
            if keep:
                report["models"][cls] += 1
            out.append((keys, [b.moved(dz) for b in keep]))
            # NB: a station marker at a published deck's interface panel was tried and reverted. Those
            # panels use sky/trigger shaders (common/junk_sky, common/trigger), so a marker at the
            # panel's centre is inside the void: q3map2 reports "Entity leaked" and writes no portal
            # file, and -vis fails. Put the marker at a chosen open-space origin on the right deck
            # instead -- the S5 brief's "record the chosen origins as invention" -- with a person.
        report["decks"][n] = {"entities": len(ents), "z": [min(zs), max(zs)] if zs else None}

    # The turbolift reaches every deck from every deck. The published decks link only to each other
    # (their menu lists ten); wherever a link is missing, one is added under the name the others use.
    names = {k.get("targetname", "").lower() for k, _ in out}
    arrival_at = {k["lwh_deck"]: k.get("origin", "0 0 0") for k, _ in out if k.get("targetname", "").endswith("_arrival")}
    for n in decks:
        for m in decks:
            name = "d%02d_tour_turbo_%02d" % (n, m)
            if m == n or name in names:
                continue
            # It needs a place inside the ship: an entity with no origin sits at the map's origin, out
            # in the void, and the compiler calls that a leak and writes no visibility data.
            out.append((collections.OrderedDict([("classname", "target_teleporter"), ("targetname", name),
                                                 ("target", "d%02d_arrival" % m), ("origin", arrival_at.get(str(n), "0 0 0")),
                                                 ("lwh_deck", str(n))]), []))
            report["turbolift_links_added"] += 1

    report["textures_substituted"] = Brush.substituted
    report["world_brushes"] = len(world)
    report["entities"] = len(out) + 1 + len(late)
    report["brush_models"] = sum(report["models"].values())
    report["model_limit"] = MODEL_LIMIT
    report["models"] = dict(report["models"].most_common())
    report["folded"] = dict(report["folded"])
    report["level_changes_left"] = sorted(set(report["level_changes_left"]))

    # No two decks may share space.
    spans = sorted((v["z"][0], v["z"][1], n) for n, v in report["decks"].items() if v["z"])
    report["overlaps"] = [[a[2], b[2]] for a, b in zip(spans, spans[1:]) if b[0] < a[1]]
    report["late"] = late
    return world_keys, world, out, report


def write_late(path, late):
    """Entities kept out of the compile (box triggers: a compiler flood-fills from every entity with
    an origin, and a trigger volume's centre is often inside a wall). tools/shipmap/inject.py adds
    them to the compiled map."""
    with open(path, "w", encoding="latin-1") as f:
        for keys, _ in late:
            f.write("{\n" + "".join('"%s" "%s"\n' % kv for kv in keys.items()) + "}\n")


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
    ap.add_argument("--decks", required=True, action="append",
                    help="directory holding deckNN.map sources; may be given more than once (a later one wins)")
    ap.add_argument("--out", required=True)
    ap.add_argument("--pitch", type=int, default=3072, help="vertical distance between decks (multiple of 1024)")
    ap.add_argument("--report")
    a = ap.parse_args(argv)

    files = {}
    for directory in a.decks:
        for path in sorted(glob.glob(os.path.join(directory, "**", "deck*.map"), recursive=True)):
            m = re.search(r"deck(\d+)\.map$", os.path.basename(path), re.I)
            if m:
                files[int(m.group(1))] = path
    if not files:
        print(f"no deckNN.map under {', '.join(a.decks)}", file=sys.stderr)
        return 2
    try:
        world_keys, world, ents, report = stitch(files, a.pitch)
    except ValueError as e:
        print(str(e), file=sys.stderr)
        return 2
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    write(a.out, world_keys, world, ents)
    late = report.pop("late")
    write_late(os.path.splitext(a.out)[0] + ".ents", late)
    if a.report:
        with open(a.report, "w") as f:
            json.dump(report, f, indent=1)

    print(f"{len(files)} decks -> {a.out}")
    print(f"  world brushes {report['world_brushes']}, entities {report['entities']}, "
          f"stray brushes dropped {report['dropped_stray_brushes']}, folded into the world {report['folded']}")
    print(f"  brush models {report['brush_models']} (engine limit {MODEL_LIMIT}): {report['models']}")
    print(f"  triggers turned from brush models into boxes: {report['triggers_boxed']}")
    print("  station markers on the published decks: none (their panels are sky/trigger brushes and a marker there leaks; see the note in stitch.py)")
    print(f"  names renamed because more than one deck uses them: {len(report['renamed'])}")
    print(f"  surfaces given a substitute for a texture the shipped game lacks: {report['textures_substituted']}")
    print(f"  turbolift links between decks: {report['turbolift_links']} rewritten, {report['turbolift_links_added']} added; level changes left as they were "
          f"(not decks of this ship): {len(report['level_changes_left'])}")
    for n, v in sorted(report["decks"].items()):
        print(f"  deck {n:2}: z {v['z'][0]:.0f} .. {v['z'][1]:.0f}")
    if report["overlaps"]:
        print(f"  OVERLAP between decks {report['overlaps']}: raise --pitch", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
