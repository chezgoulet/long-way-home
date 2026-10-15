#!/usr/bin/env python3
"""Re-dress the away site -- a **derelict interior** -- from `scav2`.

    dress_derelict.py --source build/gdk/maps/eliteforce_scav_maps/scav2.map \
        --out build/away/derelict --report build/away/derelict.json --census build/away/derelict-census.json

The design (`docs/outside-the-ship.md`, the archetype table, its **first** row) names the **derelict
interior** as a re-dress of `dn*`, `borg*`, `scav*`: *wrecked corridors, failing light, no gravity in
places*. The source is `scav2`, chosen by the survey (`docs/evidence/the-archetype-survey.md`): the
densest **single-level** interior of the four candidates. This tool is that copy. It reads the published
`scav2` source and writes `lwh_derelict.map`:

  * copies the source's own brushes on **one level** (floor 224), clamped to the window, dropping the
    machine blocks and the levels below (`COPY_ZMIN`);
  * **shells** it: a closed hull -- floor, four walls, a ceiling, no sky -- and a few **wreck** details
    (a fallen girder, a buckled plate, a burnt bulkhead) from the source's own rust and burnt materials;
  * lights it **failing**: a few dim markers, every other one out, and one low emergency strip -- never
    the outpost's warm ceiling. `--unlit` drops this AND the source's own lights, for the before-capture
    the derelict measure (`frame.py derelict`) must fail on;
  * a **no-gravity zone** (`target_gravity_change` + a trigger), the base game's own per-region mechanism.

Every texture and model this tool emits already exists in the shipped game: the parts list is a census
over the source it copies (--census). No new art.
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
X0, X1 = -640, 640
Y0, Y1 = 64, 1280
FLOOR = 224             # the level this derelict is cropped to (the source is multi-level)
ROOF = 560              # clears the tallest copied structure on this level
WALL = 16
COPY_ZMIN = FLOOR       # copy only what reaches this level; the decks below are not this site

# The source's own materials (`scav2`: a scavengers' wreck). No new art; the shell is built from the
# source's own rust, burnt panelling and posts (docs/authoring-a-location.md: reuse, name the source).
WALLTEX = "scavenger/k_basic7_b"
FLOORTEX = "scavenger/k_flat"
RIMTEX = "scavenger/k_basicpost_b"
TRIMTEX = "scavenger/k_basetrim_b"
RUSTTEX = "scavenger/k_rust_b"
BURNTEX = "scavenger/c_transwall_burn"
LAMPTEX = "scavenger/k_ceiling2_b"

DROP_TEX = ("common/nodrop", "common/sky_light", "common/clip", "common/trigger", "common/caulk")

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
    # and a crop that keeps them is a maze, not a place. A block is a brush with a body (over 64 units
    # square) that is either **floating above** this deck (its bottom above the floor) or **standing on
    # it as a body** (bottom at the floor, taller than a plate). Everything that *forms* the deck stays:
    # the floor plates and the ribs and rails are thin, and the walls reach down past the floor.
    # (This closes the hole the vantages pass found: a block whose bottom sits exactly on the floor
    # survived `lo[2] > FLOOR + 8` and blocked the stores post's outward view.)
    block = (b.hi[0] - b.lo[0]) * (b.hi[1] - b.lo[1]) > 64 * 64
    if block and (b.lo[2] > FLOOR + 8 or (b.lo[2] >= FLOOR - 8 and (b.hi[2] - b.lo[2]) > 64)):
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
    def __init__(self, source, lit=True):
        self.lit = lit
        self.report = {"copied_from": os.path.basename(source),
                       "window": {"x": [X0, X1], "y": [Y0, Y1], "floor": FLOOR, "roof": ROOF},
                       "brushes_kept": 0, "brushes_dropped": 0, "patches_dropped": 0,
                       "lights_kept": 0, "entities_dropped": 0, "waypoints": 0, "lit": lit,
                       "census": census(stitch.parse(source)),
                       "parts": collections.Counter()}
        self.world = []
        self.entities = []
        self.solids = Solids()
        self._copy_deck(stitch.parse(source))
        self._shell()
        if lit:
            self._lighting()          # --unlit drops this AND the source's own lights, for the
        self._console()               # before-capture the derelict measure must fail on
        self._crates()
        self._zerog()
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
                if self.lit and X0 <= x <= X1 and Y0 <= y <= Y1 and z >= FLOOR - 8:
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
        # a closed wreck: a floor over the levels below, four walls, a ceiling. No sky -- a derelict
        # interior is enclosed, and its failing light is the source's own, not a world beyond the hull.
        self.w(brush(((X0 - WALL, Y0 - WALL, FLOOR - WALL), (X1 + WALL, Y1 + WALL, FLOOR)), FLOORTEX), FLOORTEX)
        self.w(brush(((X0 - WALL, Y0 - WALL, FLOOR - WALL), (X0, Y1 + WALL, ROOF + WALL)), WALLTEX), WALLTEX)
        self.w(brush(((X0 - WALL, Y0 - WALL, FLOOR - WALL), (X1 + WALL, Y0, ROOF + WALL)), WALLTEX), WALLTEX)
        self.w(brush(((X0 - WALL, Y1, FLOOR - WALL), (X1 + WALL, Y1 + WALL, ROOF + WALL)), WALLTEX), WALLTEX)
        self.w(brush(((X1, Y0 - WALL, FLOOR - WALL), (X1 + WALL, Y1 + WALL, ROOF + WALL)), BURNTEX), BURNTEX)
        self.w(brush(((X0 - WALL, Y0 - WALL, ROOF), (X1 + WALL, Y1 + WALL, ROOF + WALL)), WALLTEX), WALLTEX)
        self._wreck()

    def _wreck(self):
        # the read of damage, not untidiness (docs/outside-the-ship.md: "wrecked corridors"): a fallen
        # girder across the corridor, a buckled ceiling plate, a burnt bulkhead -- the source's own rust
        # and burnt materials, placed by hand at a few spots, so the corridor is *broken*, not just empty.
        for (x, y, dx, dy) in ((X0 + 320, Y0 + 384, 128, 40), (X0 + 704, Y0 + 896, 40, 160),
                               (X1 - 480, Y0 + 512, 160, 32)):
            base = self.solids.floor_at(x, y)
            self.w(brush(((x - dx // 2, y - dy // 2, base + 8), (x + dx // 2, y + dy // 2, base + 28)), RUSTTEX), RUSTTEX)
        for (x, y) in ((X0 + 480, Y0 + 640), (X1 - 560, Y0 + 320)):
            self.w(brush(((x - 48, y - 48, ROOF - 40), (x + 48, y + 48, ROOF - 16)), BURNTEX), BURNTEX)

    def _lighting(self):
        # FAILING LIGHT (docs/outside-the-ship.md). Not the outpost's warm ceiling grid: a few dim,
        # spaced markers -- **every other one out, a failing bank** -- and one low dim emergency strip.
        # Much of the corridor is dark, and the light that is there is deliberate, which is what the
        # derelict measure (`frame.py derelict`) asserts: dark (a median ceiling), structured, and
        # **marked** (a lit pixel). It is why the room measure cannot be used here and why NO per-frame
        # dark exemption is allowed for this archetype (docs/evidence/derelict-interior.md).
        n = 0
        for x in range(X0 + 128, X1 - 64, 224):
            for y in range(Y0 + 128, Y1 - 64, 224):
                if ((x // 224) + (y // 224)) % 2:
                    continue                                   # a failing bank: every other marker out
                self.point("light", None, (x, y, FLOOR + 176), {"light": "560", "_color": "0.88 0.82 0.68"})
                self.w(brush(((x - 16, y - 16, ROOF - 20), (x + 16, y + 16, ROOF - 6)), LAMPTEX), LAMPTEX)
                n += 1
        for y in range(Y0 + 160, Y1 - 64, 192):
            self.w(brush(((X0 + 96, y, FLOOR + 8), (X0 + 112, y + 96, FLOOR + 16)), TRIMTEX), TRIMTEX)
            self.point("light", None, (X0 + 104, y + 48, FLOOR + 40), {"light": "260", "_color": "0.90 0.55 0.40"})
            n += 1
        self.report["lights_added"] = n

    def brush_ent(self, keys, lines):
        self.entities.append((collections.OrderedDict(keys), [aslines(lines)]))

    def _zerog(self):
        # "no gravity in places" (docs/outside-the-ship.md). The base game's own per-region mechanism:
        # a `trigger_multiple` fires a `target_gravity_change` whose `gravity` is 0 for the player who
        # touches it -- a ZONE, not the whole map (the source ships `target_gravity_change`; upstream
        # `g_target.cpp`). A second trigger west of the zone restores gravity on the way out.
        zx = X0 + 512
        self.point("target_gravity_change", "lwh_zerog_off", (zx, (Y0 + Y1) // 2, FLOOR + 24),
                   {"gravity": "0", "spawnflags": "0"})
        self.point("target_gravity_change", "lwh_zerog_on", (zx, (Y0 + Y1) // 2, FLOOR + 24),
                   {"gravity": "800", "spawnflags": "0"})
        self.point("info_notnull", "lwh_zerog_zone", (X0 + 640, (Y0 + Y1) // 2, FLOOR + 24))
        self.point("info_notnull", "lwh_zerog_hall", (X0 + 128, (Y0 + Y1) // 2, FLOOR + 24))
        self.brush_ent({"classname": "trigger_multiple", "targetname": "lwh_zerog_trigger",
                        "target": "lwh_zerog_off", "wait": "1", "spawnflags": "0"},
                       brush(((X0 + 384, Y0 + 224, FLOOR - 16), (X1 - 128, Y1 - 224, ROOF - 64)), "common/trigger"))
        self.brush_ent({"classname": "trigger_multiple", "targetname": "lwh_zerog_restore",
                        "target": "lwh_zerog_on", "wait": "1", "spawnflags": "0"},
                       brush(((X0, Y0, FLOOR - 16), (X0 + 320, Y1, ROOF - 64)), "common/trigger"))
        self.report["zerog_zone"] = [X0 + 384, Y0 + 224, X1 - 128, Y1 - 224]

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
        # The crate is placed **east** of the rack -- where a crate off a rack would fall -- but kept
        # **low** (below the eye line), because the stores post stands at the rack and looks east out
        # into the debris field: a crate standing tall on that side sits in the sightline and blocks
        # the outward view (the defect the vantages pass mis-attributed to a source machine block;
        # docs/evidence/the-away-site-exterior.md).
        spot = self._find_open(X1 - 420, Y1 - 300)
        if not spot:
            self.report["crates"] = None
            return
        x, y = spot
        base = self.solids.floor_at(x, y)
        self.point("misc_model", "lwh_derelict_rack", (x, y, base + 2),
                   {"model": CARGO_RACK, "angle": "90", "spawnflags": "0"})
        self.w(brush(((x + 96, y - 48, base), (x + 176, y + 32, base + 32)), RIMTEX), RIMTEX)
        self.w(brush(((x + 104, y - 40, base + 32), (x + 168, y + 24, base + 38)), TRIMTEX), TRIMTEX)
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
        for x in range(X0 + 256, X1 - 96, 64):
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
        # spot this tool has already established is open on the deck -- the console and the stores --
        # not a search that may wander off to the far side of the site. **The arrival is not a post**:
        # it is the player's spawn (`info_player_start`), not a place of work, and the site's contract
        # is two posts (the vantages pass names "the site's own two posts").
        posts = []
        for label, near in (("console", self.report.get("anchor")),
                            ("stores", self.report.get("crates"))):
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
        # the arrival: at the corridor's near end, facing in
        ax, ay = X1 - 320, (Y0 + Y1) // 2
        spot = self._find_open(ax, ay)
        if spot:
            ax, ay = spot
        az = self.solids.floor_at(ax, ay) + 24
        self.point("info_player_start", None, (ax, ay, az), {"angle": "180"})
        self.point("info_notnull", "lwh_derelict_site", (ax, ay, az),
                   {"message": "away site: a derelict interior"})
        self.report["arrival"] = [ax, ay, az]


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--source", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--name", default="lwh_derelict")
    ap.add_argument("--unlit", action="store_true",
                    help="add NO lighting and drop the source's own lights: the before-capture the "
                         "derelict measure must fail on (docs/evidence/derelict-interior.md)")
    ap.add_argument("--report")
    ap.add_argument("--census")
    a = ap.parse_args(argv)

    b = Build(a.source, lit=not a.unlit)
    os.makedirs(a.out, exist_ok=True)
    path = os.path.join(a.out, "%s.map" % a.name)
    world_keys = collections.OrderedDict([
        ("classname", "worldspawn"),
        ("message", "Away site - a derelict interior (re-dressed from scav2)"),
        ("soundSet", "forgehalls"),
        ("music", "music/elite17.mp3"),
        ("gravity", "800"),      # the map's own gravity; a zone drops it to 0 (target_gravity_change)
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
