#!/usr/bin/env python3
"""Re-dress the away site -- an outpost's apron in a debris field -- from `ctf_space`.

    dress_away_site.py --source build/gdk/maps/eliteforce_ctf_maps/ctf_space.map \
        --out build/away/generated --report build/away/dress.json --census build/away/census.json

The design (`docs/outside-the-ship.md`, the archetype table) names `ctf_space` as the source for the
**space station / outpost** archetype: "docking bays, terminals, a market". This tool is that copy.
It reads the published `ctf_space` source and writes `lwh_away_depot.map`:

  * copies the outpost's **top deck** -- the dense cluster of detail brushes around x 400..1280,
    y 672..1144 -- with its own floor plates, ribs, machinery, railings and trim lights, clamped to
    that window so nothing reaches outside it;
  * **shells** the window: solid walls west/south/north, a solid roof over the west half, and the
    east half left open to the debris field under the source's own lit sky;

The exterior is not built here. It is the source's own: `ctf_space`'s deck already stands in a debris
field under `textures/common/sky_light` (`skyParms env/junk`, `q3map_sun`), and this tool keeps that
material for the site's sky -- the ceiling over the open half, and the wall beyond the parapet.

Every texture and model this tool emits already exists in the shipped game: the parts list is a
census over the source it copies (--census). No new art.
"""

import argparse
import collections
import json
import math
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "mapgen"))
sys.path.insert(0, os.path.join(HERE, "..", "shipmap"))
from mapgen import brush  # noqa: E402
import stitch  # noqa: E402

# ---- the deck, and how it is changed -------------------------------------------------------------
# The outpost's top deck, from the source itself: the detail brushes whose tops sit at the top level
# lie x 336..1240, y 696..1120. Snapped to a 16-grid and padded to a wall line. Everything outside
# is the outpost's lower decks and its open arena: not this site, not copied.
X0, X1 = 400, 1280
Y0, Y1 = 672, 1144
FLOOR = 168              # the deck's own floor (the floor plates' top)
ROOF = 560               # clears the tallest copied structure (504) by a head
WALL = 16
PARAPET = 56             # the lip at the open edge
ROOF_X1 = 880            # the roof covers X0..ROOF_X1; the rest is open under the sky
COPY_ZMIN = FLOOR        # copy only what reaches the top deck; the decks below are not this site

SKYTEX = "common/sky_light"     # the source's own lit debris sky
WALLTEX = "forge/fmetal1"        # the source's own wall/blkhd plate (its brighter of the two)
FLOORTEX = "forge/flooring_b"
RIMTEX = "forge/fmetal1"        # the source's own machinery material
TRIMTEX = "forge/trimlight2"    # the source's own red light strip
LAMPTEX = "forge/lillight"

DROP_TEX = ("common/nodrop", "common/sky_light", "common/clip")

CARGO_RACK = "models/mapobjects/cargo/rack.md3"   # shipped; used in scav1 (named in the report)


def aslines(b):
    if isinstance(b, str):
        ls = b.split("\n")
        if ls and ls[0].strip() == "{":
            ls = ls[1:]
        if ls and ls[-1].strip() == "}":
            ls = ls[:-1]
        return ls
    return b


def fmt(v):
    return str(int(v)) if v == int(v) else ("%.4f" % v).rstrip("0").rstrip(".")


def clampf(v, a, b):
    return a if v < a else b if v > b else v


def brush_tex(lines):
    for ln in lines:
        if ln.startswith("("):
            m = stitch.FACE_TEXTURE.match(ln)
            if m:
                return m.group(2).lower()
    return ""


def clamp_line(ln):
    """Clamp a face's three plane points to the window in x and y. z is untouched: the site is one
    deck, taken as it stands."""
    if not ln.startswith("("):
        return ln
    n = [0]

    def rep(m):
        n[0] += 1
        if n[0] > 3:
            return m.group(0)
        x = clampf(float(m.group(1)), X0, X1)
        y = clampf(float(m.group(2)), Y0, Y1)
        return "( %s %s %s )" % (fmt(x), fmt(y), m.group(3))
    return stitch.POINT.sub(rep, ln)


def copy_brush(b):
    """The source brush, clamped to the window; None if it is not part of this deck."""
    if b.is_patch:
        if not (X0 <= b.lo[0] and b.hi[0] <= X1 and Y0 <= b.lo[1] and b.hi[1] <= Y1):
            return None
        return list(b.lines)
    if (b.hi[0] - b.lo[0]) > 1400 or (b.hi[1] - b.lo[1]) > 1400:
        return None                                   # the source's own sky box and ground plane
    if brush_tex(b.lines) in DROP_TEX:
        return None
    if b.hi[2] < COPY_ZMIN:
        return None                                   # below this deck: not this site
    if not (b.lo[0] < X1 and b.hi[0] > X0 and b.lo[1] < Y1 and b.hi[1] > Y0):
        return None
    # A machine block standing on the deck is dropped: the source's arena packs the deck with them,
    # and a crop that keeps them is a maze, not a place. Only blocks with a body (over 64 units
    # square) go; the thin ribs and rails, and everything that forms the deck and its walls, stay.
    if b.lo[2] > FLOOR + 8 and (b.hi[0] - b.lo[0]) * (b.hi[1] - b.lo[1]) > 64 * 64:
        return None
    return [clamp_line(ln) for ln in b.lines]


class Solids:
    def __init__(self):
        self.boxes = []

    def add(self, lines):
        lo, hi = [1e9] * 3, [-1e9] * 3
        for ln in lines:
            if not ln.startswith("("):
                continue
            for m in stitch.POINT.finditer(ln):
                for i in range(3):
                    v = float(m.group(i + 1))
                    lo[i], hi[i] = min(lo[i], v), max(hi[i], v)
        if lo[0] <= hi[0]:
            self.boxes.append((lo, hi))

    def contains(self, p, pad=16.0):
        for lo, hi in self.boxes:
            if (lo[0] - pad <= p[0] <= hi[0] + pad and lo[1] - pad <= p[1] <= hi[1] + pad
                    and lo[2] - pad <= p[2] <= hi[2] + pad):
                return True
        return False

    def box_clear(self, x, y, z, hx=20.0, zlo=24.0, zhi=40.0):
        """A point entity's own bounding box (the engine's waypoint is -12,-12,-24 .. 12,12,32) does
        not overlap a solid. A point that is merely *near* a brush is in solid to the engine, and the
        engine aborts the load on `Waypoint ... in solid!` -- so the box is what is tested."""
        for lo, hi in self.boxes:
            if (lo[0] < x + hx and hi[0] > x - hx and lo[1] < y + hx and hi[1] > y - hx
                    and lo[2] < z + zhi and hi[2] > z - zlo):
                return False
        return True

    def floor_at(self, x, y):
        best = FLOOR
        for lo, hi in self.boxes:
            if not (lo[0] - 8 <= x <= hi[0] + 8 and lo[1] - 8 <= y <= hi[1] + 8):
                continue
            if hi[2] <= FLOOR + 160 and (hi[0] - lo[0]) * (hi[1] - lo[1]) >= 48 * 48 and hi[2] > best:
                best = hi[2]
        return best


def census(ents):
    models, texs, classes = collections.Counter(), collections.Counter(), collections.Counter()
    for keys, brushes in ents:
        classes[keys.get("classname", "")] += 1
        if keys.get("model"):
            models[keys["model"]] += 1
        for b in brushes:
            for ln in b.lines:
                if ln.startswith("("):
                    m = stitch.FACE_TEXTURE.match(ln)
                    if m:
                        texs[m.group(2)] += 1
                        break
    return {"models": dict(models.most_common()), "textures": dict(texs.most_common()),
            "classes": dict(classes.most_common())}


class Build:
    def __init__(self, source):
        self.report = {"copied_from": os.path.basename(source),
                       "window": {"x": [X0, X1], "y": [Y0, Y1], "floor": FLOOR, "roof": ROOF},
                       "brushes_kept": 0, "brushes_dropped": 0, "patches_dropped": 0,
                       "lights_kept": 0, "entities_dropped": 0, "waypoints": 0,
                       "census": census(stitch.parse(source)),
                       "parts": collections.Counter()}
        self.world = []
        self.entities = []
        self.solids = Solids()
        self._copy_deck(stitch.parse(source))
        self._shell()
        self._lighting()
        self._console()
        self._crates()
        self._waypoints()

    # -- the copy ---------------------------------------------------------------------------------
    def _copy_deck(self, ents):
        for keys, brushes in ents:
            cls = keys.get("classname", "")
            if cls in ("worldspawn", "func_group"):
                for b in brushes:
                    lines = copy_brush(b)
                    if lines is None:
                        self.report["patches_dropped" if b.is_patch else "brushes_dropped"] += 1
                        continue
                    self.report["brushes_kept"] += 1
                    if b.is_patch:
                        self.report["patches_dropped"] += 0
                    self.report["parts"][brush_tex(lines) if not b.is_patch else "patch"] += 1
                    self.world.append(lines)
                    self.solids.add(lines)
                continue
            if cls == "light":
                o = keys.get("origin", "")
                try:
                    x, y, z = (float(t) for t in o.split())
                except ValueError:
                    self.report["entities_dropped"] += 1
                    continue
                if X0 <= x <= X1 and Y0 <= y <= Y1 and z >= FLOOR - 8:
                    self.entities.append((collections.OrderedDict(keys), []))
                    self.report["lights_kept"] += 1
                else:
                    self.report["entities_dropped"] += 1
                continue
            self.report["entities_dropped"] += 1     # CTF spawns, weapons, items: the arena's, not ours

    # -- new construction -------------------------------------------------------------------------
    def w(self, lines, tex=None):
        lines = aslines(lines)
        self.world.append(lines)
        self.solids.add(lines)
        if tex:
            self.report["parts"][tex] += 1

    def point(self, cls, name, origin, extra=None):
        keys = collections.OrderedDict([("classname", cls)])
        if name:
            keys["targetname"] = name
        for k, v in (extra or {}).items():
            keys[k] = v
        keys["origin"] = "%d %d %d" % origin
        self.entities.append((keys, []))

    def _shell(self):
        # one deck's floor across the whole window, so the decks below are sealed off, not walked
        self.w(brush(((X0 - WALL, Y0 - WALL, FLOOR - WALL), (X1 + WALL, Y1 + WALL, FLOOR)), FLOORTEX),
               FLOORTEX)
        self.w(brush(((X0 - WALL, Y0 - WALL, FLOOR - WALL), (X0, Y1 + WALL, ROOF + WALL)), WALLTEX),
               WALLTEX)
        self.w(brush(((X0 - WALL, Y0 - WALL, FLOOR - WALL), (X1 + WALL, Y0, ROOF + WALL)), WALLTEX),
               WALLTEX)
        self.w(brush(((X0 - WALL, Y1, FLOOR - WALL), (X1 + WALL, Y1 + WALL, ROOF + WALL)), WALLTEX),
               WALLTEX)
        # the open half: sky overhead, and sky beyond a parapet at the far edge
        self.w(brush(((ROOF_X1, Y0 - WALL, ROOF), (X1 + WALL, Y1 + WALL, ROOF + WALL)), SKYTEX), SKYTEX)
        # the sky wall that closes the open edge, and the roof over the sheltered half
        self.w(brush(((X1, Y0 - WALL, FLOOR), (X1 + WALL, Y1 + WALL, ROOF + WALL)), SKYTEX), SKYTEX)
        self.w(brush(((X0 - WALL, Y0 - WALL, ROOF), (ROOF_X1, Y1 + WALL, ROOF + WALL)), WALLTEX), WALLTEX)
        # a lip at the open edge, so a person stands at the edge instead of walking off it
        self.w(brush(((X1 - WALL, Y0 - WALL, FLOOR), (X1, Y1 + WALL, FLOOR + PARAPET)), WALLTEX), WALLTEX)
        # and the far edge of the deck is caulked: the void below the parapet is not drawn
        self._edge_lights()

    def _edge_lights(self):
        # marker lights along the parapet: the source's own lamp material, so the edge reads at a
        # distance. Placed as small boxes on the deck, not entities, so they cost nothing to bake.
        y = Y0 + 96
        while y < Y1 - 64:
            self.w(brush(((X1 - 40, y, FLOOR + PARAPET), (X1 - 8, y + 32, FLOOR + PARAPET + 24)), LAMPTEX),
                   LAMPTEX)
            y += 224

    def _lighting(self):
        # Light by function (docs/location-brief-template.md, section 6). The source's deck is lit by
        # its own red trim strips and is dim; this site is a working outpost, so the sheltered half is
        # given warm ceiling light and the open apron cool floods. The material and the colours are the
        # source's own light entities' shape, re-sited.
        n = 0
        for x in range(X0 + 112, X1 - 64, 176):
            for y in range(Y0 + 112, Y1 - 64, 176):
                self.point("light", None, (x, y, FLOOR + 168),
                           {"light": "2000", "_color": "1.00 0.94 0.82"})
                # a visible fixture at the light, the source's own lamp material: the room reads as
                # lit from somewhere, not as uniformly bright
                self.w(brush(((x - 24, y - 24, ROOF - 24), (x + 24, y + 24, ROOF - 8)), LAMPTEX), LAMPTEX)
                n += 1
        # the open apron gets cool floods high up, so the deck reads against the black sky
        for (x, y) in ((ROOF_X1 + 224, Y0 + 288), (ROOF_X1 + 224, Y1 - 288),
                       (X1 - 224, Y0 + 288), (X1 - 224, Y1 - 288)):
            self.point("light", None, (x, y, FLOOR + 288), {"light": "1400", "_color": "0.80 0.86 1.00"})
            n += 1
        # lit wall lamps along the long walls, at a person's height, so the room reads from inside it
        for x in range(X0 + 192, X1 - 128, 224):
            for y in (Y0, Y1 - 16):
                self.w(brush(((x - 32, y, FLOOR + 144), (x + 32, y + 16, FLOOR + 208)), LAMPTEX), LAMPTEX)
                self.point("light", None, (x, y + 8, FLOOR + 176), {"light": "500", "_color": "1.00 0.92 0.80"})
                n += 1
        # the edge lamps are lit, so the parapet reads as the site's edge
        y = Y0 + 112
        while y < Y1 - 64:
            self.point("light", None, (X1 - 40, y, FLOOR + PARAPET + 32),
                       {"light": "250", "_color": "1.00 0.80 0.55"})
            n += 1
            y += 224
        self.report["lights_added"] = n

    def _console(self):
        _, (ox, oy), ba = self._open_spot()
        if ox is None:
            ox, oy, ba = X0 + 320, (Y0 + Y1) // 2, 0
        base = self.solids.floor_at(ox, oy)
        self.point("info_notnull", "lwh_station_site", (ox, oy, base + 24))
        cx = ox + int(math.cos(math.radians(ba)) * 96)
        cy = oy + int(math.sin(math.radians(ba)) * 96)
        cb = self.solids.floor_at(cx, cy)
        x0, x1 = cx - 64, cx + 64
        y0, y1 = cy - 48, cy + 48
        self.w(brush(((x0, y0, cb), (x1, y1, cb + 48)), RIMTEX), RIMTEX)
        self.w(brush(((x0 + 16, y0 + 8, cb + 48), (x1 - 16, y1 - 8, cb + 80)), WALLTEX), WALLTEX)
        self.w(brush(((x0 + 24, y0 + 12, cb + 80), (x1 - 24, y1 - 12, cb + 112)), TRIMTEX), TRIMTEX)
        self.report["console"] = [cx, cy, cb]
        self.report["anchor"] = [ox, oy, base + 24]

    def _crates(self):
        # "one human trace": a shipped cargo rack left standing on the apron, and a crate with it.
        # The rack is scav1's own model (`models/mapobjects/cargo/rack.md3`), named in the report.
        spot = self._find_open(X1 - 420, Y1 - 300)
        if not spot:
            self.report["crates"] = None
            return
        x, y = spot
        base = self.solids.floor_at(x, y)
        self.point("misc_model", "lwh_away_rack", (x, y, base + 2),
                   {"model": CARGO_RACK, "angle": "90", "spawnflags": "0"})
        self.w(brush(((x + 96, y - 48, base), (x + 176, y + 32, base + 80)), RIMTEX), RIMTEX)
        self.w(brush(((x + 104, y - 40, base + 80), (x + 168, y + 24, base + 88)), TRIMTEX), TRIMTEX)
        self.report["crates"] = [x, y]

    # -- placement helpers ------------------------------------------------------------------------
    def _clear_len(self, x, y, z, ang, maxlen):
        dx, dy = math.cos(math.radians(ang)), math.sin(math.radians(ang))
        d = 0
        while d < maxlen:
            d += 16
            if self.solids.contains((x + dx * d, y + dy * d, z), pad=6):
                return d
        return maxlen

    def _open_spot(self):
        best = (-1, (None, None), 0)
        for x in range(X0 + 256, ROOF_X1 - 96, 64):
            for y in range(Y0 + 256, Y1 - 256, 64):
                f = self.solids.floor_at(x, y)
                if f > FLOOR + 48:
                    continue
                z = f + 40
                if not self.solids.box_clear(x, y, z):
                    continue
                d, ba = 0, 0
                for ang in range(0, 360, 45):
                    dd = self._clear_len(x, y, z, ang, 320)
                    if dd > d:
                        d, ba = dd, ang
                if d > best[0]:
                    best = (d, (x, y), ba)
        return best

    def _find_open(self, cx, cy, maxr=224):
        for r in range(0, maxr, 64):
            for dx, dy in ((0, 0), (r, 0), (-r, 0), (0, r), (0, -r), (r, r), (-r, r), (r, -r), (-r, -r)):
                x, y = cx + dx, cy + dy
                if not (X0 + 96 <= x <= X1 - 96 and Y0 + 96 <= y <= Y1 - 96):
                    continue
                z = self.solids.floor_at(x, y) + 24
                if self.solids.floor_at(x, y) > FLOOR + 48:
                    continue
                if any(not self.solids.box_clear(x + ox, y + oy, z)
                       for ox, oy in ((0, 0), (64, 0), (-64, 0), (0, 64), (0, -64))):
                    continue
                return (x, y)
        return None

    def _waypoints(self):
        cand = [(x, y) for x in range(X0 + 64, X1 - 32, 96) for y in range(Y0 + 64, Y1 - 32, 96)]
        seen, final = set(), []
        for (x, y) in cand:
            key = (round(x / 64), round(y / 64))
            if key in seen:
                continue
            z = self.solids.floor_at(x, y) + 24
            if self.solids.floor_at(x, y) > FLOOR + 48:
                continue                                  # a ledge a body cannot climb: not a place
            if not self.solids.box_clear(x, y, z):
                continue
            seen.add(key)
            final.append((int(x), int(y), int(z)))
        for (x, y, z) in final:
            self.point("waypoint", None, (x, y, z))
        self.report["waypoints"] = len(final)
        # posts: the crew layer reads `waypoint_navgoal_*` as the places a person can work. Each is a
        # spot this tool has already established is open on the deck -- the console, the stores, the
        # arrival -- not a search that may wander off to the far side of the site.
        posts = []
        for label, near in (("console", self.report.get("anchor")),
                            ("stores", self.report.get("crates")),
                            ("arrival", (X1 - 320, (Y0 + Y1) // 2))):
            if not near:
                continue
            spot = self._find_open(int(near[0]), int(near[1]))
            if not spot:
                continue
            x, y = spot
            z = self.solids.floor_at(x, y) + 24
            posts.append((x, y))
            self.point("waypoint_navgoal", "lwh_post_%s" % label, (x, y, z),
                       {"message": "%s" % label})
        self.report["posts"] = posts
        # the arrival: on the apron, facing the shelter
        ax, ay = X1 - 320, (Y0 + Y1) // 2
        spot = self._find_open(ax, ay)
        if spot:
            ax, ay = spot
        az = self.solids.floor_at(ax, ay) + 24
        self.point("info_player_start", None, (ax, ay, az), {"angle": "180"})
        self.point("info_notnull", "lwh_away_site", (ax, ay, az),
                   {"message": "away site: an outpost in a debris field"})
        self.report["arrival"] = [ax, ay, az]


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--source", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--name", default="lwh_away_depot")
    ap.add_argument("--report")
    ap.add_argument("--census")
    a = ap.parse_args(argv)

    b = Build(a.source)
    os.makedirs(a.out, exist_ok=True)
    path = os.path.join(a.out, "%s.map" % a.name)
    world_keys = collections.OrderedDict([
        ("classname", "worldspawn"),
        ("message", "Away site - an outpost in a debris field (re-dressed from ctf_space)"),
        ("soundSet", "forgehalls"),
        ("music", "music/elite17.mp3"),
        ("gravity", "800"),
    ])
    with open(path, "w", encoding="latin-1") as f:
        f.write("{\n" + "".join('"%s" "%s"\n' % kv for kv in world_keys.items()))
        for lines in b.world:
            f.write("{\n" + "\n".join(lines) + "\n}\n")
        f.write("}\n")
        for i, (keys, brushes) in enumerate(b.entities, 1):
            f.write("// entity %d\n{\n" % i)
            for k, v in keys.items():
                f.write('"%s" "%s"\n' % (k, v))
            for lines in brushes:
                f.write("{\n" + "\n".join(lines) + "\n}\n")
            f.write("}\n")
    b.report["parts"] = dict(b.report["parts"])
    if a.report:
        with open(a.report, "w") as f:
            json.dump(b.report, f, indent=1)
    if a.census:
        with open(a.census, "w") as f:
            json.dump(b.report["census"], f, indent=1)
    r = b.report
    print("%s <- %s" % (a.name, a.source))
    print("  window x %d..%d y %d..%d, floor %d roof %d" % (X0, X1, Y0, Y1, FLOOR, ROOF))
    print("  world brushes %d, brushes kept %d/%d, lights kept %d, waypoints %d, posts %s"
          % (len(b.world), r["brushes_kept"], r["brushes_kept"] + r["brushes_dropped"],
             r["lights_kept"], r["waypoints"], r["posts"]))
    print("  wrote %s" % path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
